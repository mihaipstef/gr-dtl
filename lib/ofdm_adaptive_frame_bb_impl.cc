/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_frame_bb_impl.h"
#include <algorithm>

namespace gr {
namespace dtl {


using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_frame_bb")

static const pmt::pmt_t FRAME_COUNT_KEY = pmt::mp("frame_count_key");
static const pmt::pmt_t MONITOR_PORT = pmt::mp("monitor");

ofdm_adaptive_frame_bb::sptr
ofdm_adaptive_frame_bb::make(const std::string& len_tag_key,
                             const std::vector<constellation_type_t>& constellations,
                             size_t frame_len,
                             size_t n_payload_carriers,
                             std::string frames_fname,
                             int max_empty_frames)
{
    return std::make_shared<ofdm_adaptive_frame_bb_impl>(len_tag_key,
                                                         constellations,
                                                         frame_len,
                                                         n_payload_carriers,
                                                         frames_fname,
                                                         max_empty_frames);
}


ofdm_adaptive_frame_bb_impl::ofdm_adaptive_frame_bb_impl(
    const std::string& len_tag_key,
    const std::vector<constellation_type_t>& constellations,
    size_t frame_len,
    size_t n_payload_carriers,
    string frames_fname,
    int max_empty_frames)
    : block("ofdm_adaptive_frame_bb",
            io_signature::make(1, 1, sizeof(char)),
            io_signature::make(1, 1, sizeof(char))),
      d_constellation(constellation_type_t::QAM16),
      d_fec_scheme(0),
      d_tag_offset(0),
      d_frame_len(frame_len),
      d_packet_len_tag(pmt::mp(len_tag_key)),
      d_payload_carriers(n_payload_carriers),
      d_waiting_full_frame(false),
      d_waiting_for_input(false),
      d_max_empty_frames(max_empty_frames),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF),
      d_frame_count(0),
      d_has_fec(true),
      d_consecutive_empty_frames(0),
      d_tb_offset(0),
      d_tb_len(0)
{
    this->message_port_register_in(pmt::mp("feedback"));
    this->set_msg_handler(pmt::mp("feedback"),
                          [this](pmt::pmt_t msg) { this->process_feedback(msg); });
    d_bps = get_bits_per_symbol(d_constellation);
    set_min_noutput_items(frame_length());
    // d_bytes = input_length(d_frame_len, d_payload_carriers, d_bps);
    size_t frame_buffer_max_len =
        d_frame_len * d_payload_carriers * get_max_bps(constellations).second;
    d_frame_buffer.resize(frame_buffer_max_len);
    message_port_register_out(MONITOR_PORT);
    d_frame_store = frame_file_store(frames_fname);
}


void ofdm_adaptive_frame_bb_impl::process_feedback(pmt::pmt_t feedback)
{

    // If there is FEC, constellation is passed with the transport block
    // to keep code rate and constellation in sync
    if (d_has_fec) {
        return;
    }

    if (pmt::is_dict(feedback)) {
        if (pmt::dict_has_key(feedback, feedback_constellation_key())) {
            constellation_type_t constellation =
                static_cast<constellation_type_t>(pmt::to_long(pmt::dict_ref(
                    feedback,
                    feedback_constellation_key(),
                    pmt::from_long(static_cast<int>(constellation_type_t::BPSK)))));
            int bps = get_bits_per_symbol(constellation);
            // Update constellation only if valid data received
            if (bps) {
                d_constellation = constellation;
                d_bps = bps;
            }
        }
        if (pmt::dict_has_key(feedback, fec_feedback_key())) {
            d_fec_scheme = pmt::to_long(
                pmt::dict_ref(feedback, fec_feedback_key(), pmt::from_long(0)));
        }
    }
    DTL_LOG_DEBUG("process_feedback: d_constellation={}, d_fec_scheme={}",
                  static_cast<int>(d_constellation),
                  d_fec_scheme);
}


void ofdm_adaptive_frame_bb_impl::forecast(int noutput_items,
                                           gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = 0;
}

std::pair<int, int> ofdm_adaptive_frame_bb_impl::find_fec_tags(uint64_t start_idx,
                                                               uint64_t end_idx,
                                                               vector<tag_t>& tags)
{
    vector<tag_t> all_tags_in_range;
    get_tags_in_window(all_tags_in_range, 0, start_idx, end_idx);

    int64_t offset = -1;
    int len = -1;
    unsigned int check = 0;

    for (auto& tag : tags) {
        if (tag.key == fec_key()) {
            offset = tag.offset - nitems_read(0) - start_idx;
            assert(offset >= 0);
            tags.push_back(tag);
            check |= 1;
        } else if (tag.key == fec_tb_payload_key()) {
            tags.push_back(tag);
            len = pmt::to_long(tag.value);
            check |= 2;
        }
        if (check == 3) {
            break;
        }
    }
    if (check == 3) {
        return make_pair(static_cast<int>(offset), len);
    }
    return make_pair(-1, -1);
}


void ofdm_adaptive_frame_bb_impl::frame_out(const unsigned char* in,
                                            int nbytes_in,
                                            unsigned char* out,
                                            int nsyms_out,
                                            repack& repacker,
                                            bool crc,
                                            int& read_index,
                                            int& write_index)
{
    DTL_LOG_DEBUG("frame_out payload_len={}", nbytes_in);

    assert(nbytes_in <= d_frame_in_bytes);

    std::uniform_int_distribution<> rnd_bytes_dist(0, 255);

    // Copy frame input bytes, ...
    memcpy(&d_frame_buffer[0], &in[read_index], nbytes_in);

    int bytes_read = nbytes_in;

    // If CRC...
    if (crc) {
        // ... append CRC ...
        d_crc.append_crc(&d_frame_buffer[0], nbytes_in);
        bytes_read += d_crc.get_crc_len();
    }

    // If frame not full...
    if (bytes_read < d_frame_in_bytes) {
        // ... pad with random bytes.
        rand_pad(
            &d_frame_buffer[bytes_read], d_frame_in_bytes - bytes_read, rnd_bytes_dist);
    }
    // Repack frame buffer and output
    int frame_out_symbols =
        repacker.repack_lsb_first(const_cast<const unsigned char*>(&d_frame_buffer[0]),
                                  nsyms_out,
                                  &out[write_index],
                                  true);
    assert(frame_out_symbols == nsyms_out);
    d_consecutive_empty_frames = 0;
    // update indexes
    read_index += nsyms_out;
    write_index += frame_out_symbols;
}


int ofdm_adaptive_frame_bb_impl::general_work(int noutput_items,
                                              gr_vector_int& ninput_items,
                                              gr_vector_const_void_star& input_items,
                                              gr_vector_void_star& output_items)
{
    gr::thread::scoped_lock guard(d_setlock);

    auto in = static_cast<const unsigned char*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    int read_index = 0;
    int write_index = 0;
    int frame_out_symbols = 0;
    int expected_frame_symbols = 0;
    std::uniform_int_distribution<> rnd_bytes_dist(0, 255);

    DTL_LOG_DEBUG("work: d_frame_len={}, d_payload_carriers={}, "
                  "noutput_items={}, nitems_written={}, ninput_items={}",
                  d_frame_len,
                  d_payload_carriers,
                  noutput_items,
                  nitems_written(0),
                  ninput_items[0]);

    while (write_index < noutput_items) {

        int frame_payload = -1;


        int frame_bits = 0;
        vector<tag_t> fec_tags;

        // keep constellation during one frame or one TB (depending if FEC is present)
        constellation_type_t cnst = d_constellation;
        unsigned char bps = get_bits_per_symbol(cnst);

        if (d_has_fec) {
            // Calculate input frame size.
            // Number of bytes to be picked from input - each frame is carrying an integer
            // number of bytes.
            d_frame_in_bytes = d_frame_len * d_payload_carriers * bps / 8;
            frame_bits = d_frame_in_bytes * 8;
            // Looking for FEC tags - mark the begining of a TB
            pair<int, int> tb =
                find_fec_tags(read_index, read_index + d_frame_in_bytes, fec_tags);
            if (tb.first >= 0) {
                d_tb_offset = tb.first;
                d_tb_len = tb.second;
            }
        } else {
            // Leave room for CRC
            d_frame_in_bytes =
                d_frame_len * d_payload_carriers * bps / 8 - d_crc.get_crc_len();
            frame_bits = d_frame_in_bytes * 8 + d_crc.get_crc_len() * 8;
        }

        if (cnst == constellation_type_t::UNKNOWN) {
            throw runtime_error("constellation was not set correctly");
        }

        std::uniform_int_distribution<> rnd_symbols_dist(0, pow(2, bps) - 1);

        // expected output symbols
        expected_frame_symbols = frame_bits / bps;
        if (frame_bits % bps) {
            ++expected_frame_symbols;
        }

        // If there is not enough room in the output buffer...
        if (write_index + expected_frame_symbols > noutput_items) {
            // ...nothing we can do.
            break;
        }

        repack repacker(bps, 8);

        // If frame payload already set - we have to enforce the rest of the TB
        // in current frame with padding because constellation changed
        if (frame_payload >= 0) {
            frame_out(&in[read_index],
                      frame_payload,
                      &out[write_index],
                      expected_frame_symbols,
                      repacker,
                      !d_has_fec,
                      read_index,
                      write_index);
            // Overwrite the constellation for next frame
            d_constellation = cnst;
        } else {
            // If there is something to consume...
            if (read_index < ninput_items[0]) {

                d_waiting_for_input = false;

                int next_frame_nbytes =
                    min({ d_frame_in_bytes, ninput_items[0] - read_index });
                // ... output a frame.
                frame_out(&in[read_index],
                            next_frame_nbytes,
                            &out[write_index],
                            expected_frame_symbols,
                            repacker,
                            !d_has_fec,
                            read_index,
                            write_index);
                d_waiting_full_frame = false;
                frame_payload = next_frame_nbytes;
            // If there is nothing to consume
            } else {
                frame_payload = 0;
                if (d_waiting_for_input) {
                    if (d_max_empty_frames >= 0 && d_consecutive_empty_frames == d_max_empty_frames) {
                        return WORK_DONE;
                    } else {
                        // Fill with empty frame
                        DTL_LOG_DEBUG("empty frame");
                        rand_pad(
                            &out[write_index], expected_frame_symbols, rnd_symbols_dist);
                        write_index += expected_frame_symbols;
                        frame_out_symbols = expected_frame_symbols;
                        d_waiting_for_input = false;
                        ++d_consecutive_empty_frames;
                        break;
                    }
                } else {
                    d_waiting_for_input = true;
                    break;
                }
            }
        }

        // add tags
        if (d_tag_offset + expected_frame_symbols <= nitems_written(0) + write_index) {

            DTL_LOG_DEBUG("add tags: offset={}, "
                          "frame_in_bytes={}, "
                          "read_index={}, write_index={}, expected_frame={}, payload={}, "
                          "frame_count={}, constelation={}",
                          d_tag_offset,
                          d_frame_in_bytes,
                          read_index,
                          write_index,
                          expected_frame_symbols,
                          frame_payload,
                          d_frame_count,
                          static_cast<int>(cnst));

            // Add TSB length tag - frame payload length in symbols
            add_item_tag(0,
                         d_tag_offset,
                         d_packet_len_tag,
                         pmt::from_long(expected_frame_symbols));

            // Add constellation tag - constellation used for the frame
            add_item_tag(0,
                         d_tag_offset,
                         get_constellation_tag_key(),
                         pmt::from_long(static_cast<int>(cnst)));
            // Add FEC tags
            if (d_has_fec) {
                for (auto& tag : fec_tags) {
                    add_item_tag(0, d_tag_offset, tag.key, tag.value);
                }
                add_item_tag(
                    0, d_tag_offset, payload_length_key(), pmt::from_long(frame_payload));
            } else {
                // TODO: make frame payload wo CRC
                // Add transported payload tag - number of payload bytes carried by the
                // frame (including CRC)
                add_item_tag(0,
                             d_tag_offset,
                             payload_length_key(),
                             pmt::from_long(frame_payload + d_crc.get_crc_len()));
            }
            d_tag_offset += expected_frame_symbols;
            d_frame_store.store(frame_payload,
                                d_frame_count & 0xFFF,
                                reinterpret_cast<char*>(&d_frame_buffer[0]));
            ++d_frame_count;
            pmt::pmt_t monitor_msg = pmt::make_dict();
            monitor_msg = pmt::dict_add(
                monitor_msg, FRAME_COUNT_KEY, pmt::from_long(d_frame_count));
            message_port_pub(MONITOR_PORT, monitor_msg);
        }
    }

    DTL_LOG_DEBUG("work: consumed={}, produced={}", read_index, write_index);
    consume_each(read_index);
    return write_index;
}

void ofdm_adaptive_frame_bb_impl::rand_pad(unsigned char* buf,
                                           size_t len,
                                           std::uniform_int_distribution<>& dist)
{
    std::mt19937 gen(std::random_device{}());
    for (unsigned int i = 0; i < len; ++i) {
        buf[i] = dist(gen);
    }
}

bool ofdm_adaptive_frame_bb_impl::start()
{
    d_tag_offset = 0;
    return true;
}

size_t ofdm_adaptive_frame_bb_impl::frame_length()
{
    return d_frame_len * d_payload_carriers;
}


size_t ofdm_adaptive_frame_bb_impl::frame_length_bits(size_t frame_len,
                                                      size_t n_payload_carriers,
                                                      size_t bits_per_symbol)
{
    size_t n_bits =
        d_frame_len * d_payload_carriers * get_bits_per_symbol(d_constellation);
    return n_bits;
}

void ofdm_adaptive_frame_bb_impl::set_constellation(constellation_type_t constellation)
{
    d_constellation = constellation;
    // d_bps = get_bits_per_symbol(d_constellation);
}

} /* namespace dtl */
} /* namespace gr */
