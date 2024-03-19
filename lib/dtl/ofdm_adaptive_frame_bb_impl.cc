/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <algorithm>
#include <gnuradio/testbed/logger.h>
#include "ofdm_adaptive_frame_bb_impl.h"
#include <thread>

namespace gr {
namespace dtl {


using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_frame_bb")

static const pmt::pmt_t FRAME_COUNT_KEY = pmt::mp("frame_count_key");
static const pmt::pmt_t FRAME_PAYLOAD_KEY = pmt::mp("frame_payload_key");
static const pmt::pmt_t MONITOR_PORT = pmt::mp("monitor");

ofdm_adaptive_frame_bb::sptr
ofdm_adaptive_frame_bb::make(const std::string& len_tag_key,
                             const std::vector<constellation_type_t>& constellations,
                             size_t frame_len,
                             double frame_rate,
                             size_t n_payload_carriers,
                             std::string frames_fname,
                             int max_empty_frames)
{
    return std::make_shared<ofdm_adaptive_frame_bb_impl>(len_tag_key,
                                                         constellations,
                                                         frame_len,
                                                         frame_rate,
                                                         n_payload_carriers,
                                                         frames_fname,
                                                         max_empty_frames);
}


ofdm_adaptive_frame_bb_impl::ofdm_adaptive_frame_bb_impl(
    const std::string& len_tag_key,
    const std::vector<constellation_type_t>& constellations,
    size_t frame_len,
    double frame_rate,
    size_t n_payload_carriers,
    string frames_fname,
    int max_empty_frames)
    : block("ofdm_adaptive_frame_bb",
            io_signature::make(1, 1, sizeof(char)),
            io_signature::make(1, 1, n_payload_carriers * frame_len * sizeof(char))),
      d_constellation(constellation_type_t::BPSK),
      d_fec_scheme(0),
      d_tag_offset(0),
      d_frame_len(frame_len),
      d_len_key(pmt::mp(len_tag_key)),
      d_payload_carriers(n_payload_carriers),
      d_waiting_for_input(false),
      d_max_empty_frames(max_empty_frames),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF),
      d_frame_count(0),
      d_frame_in_bytes(0),
      d_consecutive_empty_frames(0),
      d_frame_duration(std::chrono::duration<double>(1.0/frame_rate)),
      d_feedback_cnst(constellation_type_t::UNKNOWN),
      d_frame_capacity(n_payload_carriers * frame_len)
{
    this->message_port_register_in(pmt::mp("feedback"));
    this->set_msg_handler(pmt::mp("feedback"),
                          [this](pmt::pmt_t msg) { this->process_feedback(msg); });
    this->message_port_register_in(pmt::mp("header"));
    this->set_msg_handler(pmt::mp("header"),
                          [this](pmt::pmt_t msg) { this->process_feedback_header(msg); });
    d_bps = get_bits_per_symbol(d_constellation);
    set_min_noutput_items(1);
    // d_bytes = input_length(d_frame_len, d_payload_carriers, d_bps);
    size_t frame_buffer_max_len =
        d_frame_len * d_payload_carriers * get_max_bps(constellations).second;
    d_frame_buffer.resize(frame_buffer_max_len);
    message_port_register_out(MONITOR_PORT);
    d_frame_store = frame_file_store(frames_fname);
}

void ofdm_adaptive_frame_bb_impl::process_feedback(pmt::pmt_t feedback)
{
    if (pmt::is_dict(feedback)) {
        DTL_LOG_DEBUG("process_feedback: is dict");
        if (pmt::dict_has_key(feedback, feedback_constellation_key())) {
            constellation_type_t constellation =
                static_cast<constellation_type_t>(pmt::to_long(pmt::dict_ref(
                    feedback,
                    feedback_constellation_key(),
                    pmt::from_long(static_cast<int>(constellation_type_t::BPSK)))));
            int bps = get_bits_per_symbol(constellation);
            DTL_LOG_DEBUG("process_feedback: has key {}", (int)constellation);

            // Update constellation only if valid data received
            if (bps) {
                d_feedback_cnst = constellation;
            }
        }
    }
    DTL_LOG_DEBUG("process_feedback: d_constellation={}",
                  static_cast<int>(d_feedback_cnst));
}

void ofdm_adaptive_frame_bb_impl::process_feedback_header(pmt::pmt_t header_data)
{
    if (pmt::is_dict(header_data)) {
        if (pmt::dict_has_key(header_data, feedback_constellation_key())) {
            constellation_type_t constellation =
                static_cast<constellation_type_t>(pmt::to_long(pmt::dict_ref(
                    header_data,
                    feedback_constellation_key(),
                    pmt::from_long(static_cast<int>(constellation_type_t::BPSK)))));
            int bps = get_bits_per_symbol(constellation);
            // Update constellation only if valid data received
            if (bps) {
                d_constellation = constellation;
                d_bps = bps;
            }
        }
    }
    DTL_LOG_DEBUG("process_feedback_header: d_constellation={}",
                  static_cast<int>(d_constellation));
}


void ofdm_adaptive_frame_bb_impl::forecast(int noutput_items,
                                           gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = 0;
}

int ofdm_adaptive_frame_bb_impl::frame_out(const unsigned char* in,
                                            int nbytes_in,
                                            unsigned char* out,
                                            int nsyms_out,
                                            repack& repacker)
{

    assert(nbytes_in <= d_frame_in_bytes);

    // Copy frame input bytes, ...
    memcpy(&d_frame_buffer[0], in, nbytes_in);

    int bytes_read = nbytes_in;

    // Append CRC ...
    d_crc.append_crc(&d_frame_buffer[0], nbytes_in);
    bytes_read += d_crc.get_crc_len();

    // If frame not full...
    if (bytes_read < d_frame_in_bytes) {
        // ... pad with random bytes.
        rand_pad(
            &d_frame_buffer[bytes_read], d_frame_in_bytes - bytes_read, 8);
    }
    // Repack frame buffer and output
    int frame_out_symbols =
        repacker.repack_lsb_first(const_cast<const unsigned char*>(&d_frame_buffer[0]),
                                  d_frame_in_bytes + d_crc.get_crc_len(),
                                  out);
    //assert(frame_out_symbols == d_frame_in_bytes);
    // update indexes
    d_consecutive_empty_frames = 0;
    DTL_LOG_DEBUG("frame_out payload_len={}, d_frame_in_bytes={}, frame_out_symbols={}", nbytes_in, d_frame_in_bytes, frame_out_symbols);
    return frame_out_symbols;
}


int ofdm_adaptive_frame_bb_impl::general_work(int noutput_items,
                                              gr_vector_int& ninput_items,
                                              gr_vector_const_void_star& input_items,
                                              gr_vector_void_star& output_items)
{
    gr::thread::scoped_lock guard(d_setlock);

    auto in = static_cast<const unsigned char*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    auto expected_time = d_start_time + d_frame_count * d_frame_duration;
    auto now = std::chrono::steady_clock::now();
    if (now < expected_time) {
        std::this_thread::sleep_until(expected_time);
    }

    DTL_LOG_DEBUG("work: d_frame_len={}, d_payload_carriers={}, "
                  "noutput_items={}, nitems_written={}, ninput_items={}",
                  d_frame_len,
                  d_payload_carriers,
                  noutput_items,
                  nitems_written(0),
                  ninput_items[0]);


    if (noutput_items >= 1) {

        if (ninput_items[0] == 0) {
            // Empty frame
            if (d_max_empty_frames >= 0 && d_consecutive_empty_frames == d_max_empty_frames) {
                return WORK_DONE;
            } else {
                constellation_type_t cnst = d_constellation; // makes sure is the same constellation in this frame
                unsigned char bps = get_bits_per_symbol(cnst);
                rand_pad(
                    out, d_frame_capacity, get_bits_per_symbol(cnst));
                add_tags(0, d_frame_capacity, cnst);
                ++d_consecutive_empty_frames;
                ++d_frame_count;
                return 1;
            }
        } else {
            // Consume input
            int read_index = 0;
            int write_index = 0;
            bool wait_next_work = false;
            int produced = 0;
            int produced_payload = 0;
            int frame_syms = 0;

            d_consecutive_empty_frames = 0;

            while (read_index < ninput_items[0] && produced < noutput_items && !wait_next_work) {

                int frame_payload = -1;

                int frame_bits = 0;

                // keep constellation during one frame or one TB (depending if FEC is present)
                constellation_type_t cnst = d_constellation;
                unsigned char bps = get_bits_per_symbol(cnst);
                repack repacker(8, bps);

                // Leave room for CRC
                d_frame_in_bytes =
                    d_frame_len * d_payload_carriers * bps / 8 - d_crc.get_crc_len();
                frame_bits = d_frame_in_bytes * 8 + d_crc.get_crc_len() * 8;

                if (cnst == constellation_type_t::UNKNOWN) {
                    throw runtime_error("constellation was not set correctly");
                }

                // expected output symbols
                frame_syms = frame_bits / bps;
                if (frame_bits % bps) {
                    ++frame_syms;
                }

                int next_frame_nbytes = consumer.advance(
                    ninput_items[0],
                    d_frame_in_bytes,
                    read_index,
                    [this](int offset) {
                        std::vector<tag_t> tags;
                        this->get_tags_in_window(tags, 0, offset, offset + 1);
                        auto len_tag = find_tag(tags, d_len_key);
                        if (len_tag == tags.end()) {
                            return std::optional<tag_t>{};
                        }
                        return std::optional<tag_t>(*len_tag);
                    });

                // ... output a frame.
                int frame_out_symbols = frame_out(&in[read_index],
                            next_frame_nbytes,
                            &out[write_index],
                            frame_syms,
                            repacker);
                assert(frame_out_symbols == frame_syms);
                frame_payload = next_frame_nbytes;
                read_index += next_frame_nbytes;
                write_index += frame_out_symbols;

                d_waiting_for_input = true;

                // add tags
                add_tags(frame_payload, frame_syms, cnst);
                assert(d_tag_offset == nitems_written(0) + write_index/frame_syms);

                produced_payload += frame_payload;
                d_frame_store.store(frame_payload,
                                    d_frame_count & 0xFFF,
                                    reinterpret_cast<char*>(&d_frame_buffer[0]));
                ++d_frame_count;
                ++produced;

                pmt::pmt_t monitor_msg = pmt::make_dict();
                monitor_msg = pmt::dict_add(
                    monitor_msg, FRAME_COUNT_KEY, pmt::from_long(d_frame_count));
                monitor_msg = pmt::dict_add(
                    monitor_msg, FRAME_COUNT_KEY, pmt::from_long(frame_payload));
                message_port_pub(MONITOR_PORT, monitor_msg);
                if (produced) {
                    break;
                }
            }

            DTL_LOG_DEBUG("work: consumed={}, produced={}, write_index={}, produced_payload={}", read_index, produced, write_index, produced_payload);
            consume_each(read_index);
            return produced;
        }
    } else {
        return 0;
    }
}


void ofdm_adaptive_frame_bb_impl::add_tags(int payload, int frame_syms, constellation_type_t cnst) {
        DTL_LOG_DEBUG("add tags: offset={}, "
                        " payload={}, "
                        "frame_count={}, constelation={}",
                        d_tag_offset,
                        payload,
                        d_frame_count,
                        static_cast<int>(cnst));

            // Add TSB length tag - frame payload length in symbols
            add_item_tag(0,
                         d_tag_offset,
                         d_len_key,
                         pmt::from_long(frame_syms));

            // Add constellation tag - constellation used for the frame
            add_item_tag(0,
                         d_tag_offset,
                         get_constellation_tag_key(),
                         pmt::from_long(static_cast<int>(cnst)));
            add_item_tag(0,
                        d_tag_offset,
                        feedback_constellation_key(),
                        pmt::from_long(static_cast<int>(d_feedback_cnst) & 0xf));
            if (payload) {
                // Add transported payload tag - number of payload bytes carried by the
                // frame (including CRC) - if there are any
                add_item_tag(0,
                                d_tag_offset,
                                payload_length_key(),
                                pmt::from_long(payload + d_crc.get_crc_len()));
            } else {
                add_item_tag(0,
                                d_tag_offset,
                                payload_length_key(),
                                pmt::from_long(0));
            }
            ++d_tag_offset;

}


void ofdm_adaptive_frame_bb_impl::rand_pad(unsigned char* buf,
                                           size_t len,
                                           int bps)
{
    std::uniform_int_distribution<> dist(0, pow(2, bps) - 1);
    std::mt19937 gen(std::random_device{}());
    for (unsigned int i = 0; i < len; ++i) {
        buf[i] = dist(gen);
    }
}

bool ofdm_adaptive_frame_bb_impl::start()
{
    d_tag_offset = 0;
    d_start_time = std::chrono::steady_clock::now();
    d_frame_count = 0;
    return block::start();
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
