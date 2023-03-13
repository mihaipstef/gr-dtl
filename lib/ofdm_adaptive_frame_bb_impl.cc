/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_frame_bb_impl.h"
#include "repack.h"
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
                             std::string frames_fname)
{
    return std::make_shared<ofdm_adaptive_frame_bb_impl>(
        len_tag_key, constellations, frame_len, n_payload_carriers, frames_fname);
}


ofdm_adaptive_frame_bb_impl::ofdm_adaptive_frame_bb_impl(
    const std::string& len_tag_key,
    const std::vector<constellation_type_t>& constellations,
    size_t frame_len,
    size_t n_payload_carriers,
    string frames_fname)
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
      d_stop_no_input(true),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF),
      d_frame_count(0)
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
    if (!frames_fname.empty()) {
        d_frame_store = frame_file_store(frames_fname);
    }
}


void ofdm_adaptive_frame_bb_impl::process_feedback(pmt::pmt_t feedback)
{
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
        if (pmt::dict_has_key(feedback, feedback_fec_key())) {
            d_fec_scheme = pmt::to_long(
                pmt::dict_ref(feedback, feedback_fec_key(), pmt::from_long(0)));
        }
    }
    DTL_LOG_DEBUG("process_feedback: d_constellation={}, d_fec_scheme={}",
                  static_cast<int>(d_constellation),
                  d_fec_scheme);
}


void ofdm_adaptive_frame_bb_impl::forecast(int noutput_items,
                                           gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = 1;
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
    repack repacker;
    int frame_out_symbols = 0;
    int expected_frame_symbols = 0;
    bool frame_count_change = false;

    DTL_LOG_DEBUG("work: d_frame_len={}, d_payload_carriers={}, "
                  "noutput_items={}, nitems_written={}, ninput_items={}",
                  d_frame_len,
                  d_payload_carriers,
                  noutput_items,
                  nitems_written(0),
                  ninput_items[0]);

    while (write_index < noutput_items) {
        // keep constellation during one frame
        constellation_type_t cnst = d_constellation;
        unsigned char bps = get_bits_per_symbol(cnst);

        int frame_payload = 0;

        // Calculate input frame size.
        // Number of bytes to be picked from input - each frame is carrying an integer
        // number of bytes.
        int frame_in_bytes = d_frame_len * d_payload_carriers * bps / 8 - d_crc.get_crc_len();
        int frame_bits = frame_in_bytes * 8 + d_crc.get_crc_len() * 8;
        // expected output symbols, including CRC
        expected_frame_symbols = frame_bits / bps;
        if (frame_bits % bps) {
            ++expected_frame_symbols;
        }
        repacker.set_indexes(0, 0);

        // If there is something to consume...
        if (read_index < ninput_items[0]) {

            d_waiting_for_input = false;

            // ... and enough room for a frame in the output buffer
            if (write_index + expected_frame_symbols <= noutput_items) {

                // If we have enough input for a frame ...
                if (read_index + frame_in_bytes <= ninput_items[0]) {
                    // ... copy frame input bytes, ...
                    memcpy(&d_frame_buffer[0], &in[read_index], frame_in_bytes);
                    // ... append CRC ...
                    d_crc.append_crc(&d_frame_buffer[0], frame_in_bytes);
                    // ... and repack a full frame.
                    frame_out_symbols = repacker.repack_lsb_first(
                        const_cast<const unsigned char*>(&d_frame_buffer[0]),
                        frame_in_bytes + d_crc.get_crc_len(),
                        &out[write_index],
                        bps,
                        true);
                    assert(frame_out_symbols == expected_frame_symbols);
                    d_waiting_full_frame = false;
                    // update indexes and offset
                    frame_payload = frame_in_bytes;
                    read_index += frame_payload;
                    write_index += frame_out_symbols;
                    // If we do not have enough input for a frame but there is enough
                    // output...
                } else {
                    // ... wait for next cycle if new input available.
                    // If we were waiting for new input in previous cycle...
                    if (d_waiting_full_frame) {
                        // ... copy frame input bytes, ...
                        memcpy(&d_frame_buffer[0],
                               &in[read_index],
                               ninput_items[0] - read_index);
                        // ... append CRC ...
                        d_crc.append_crc(&d_frame_buffer[0],
                                   ninput_items[0] - read_index);
                        // ...repack what we have.
                        frame_out_symbols =
                            repacker.repack_lsb_first(const_cast<const unsigned char*>(&d_frame_buffer[0]),
                                                      ninput_items[0] - read_index + d_crc.get_crc_len(),
                                                      &out[write_index],
                                                      bps,
                                                      true);
                        // update indexes
                        frame_payload = ninput_items[0] - read_index;
                        read_index += frame_payload;
                        write_index += expected_frame_symbols;
                        // frame_out_symbols = expected_frame_symbols;
                        d_waiting_full_frame = false;
                    } else {
                        d_waiting_full_frame = true;
                    }
                }
                // If there is no room in the output buffer
            } else {
                break;
            }
            // If there is nothing to consume
        } else {
            if (d_waiting_for_input) {
                if (d_stop_no_input) {
                    return WORK_DONE;
                } else {
                    // Fill with empty frame
                    memset(&out[write_index], 0, expected_frame_symbols);
                    write_index += expected_frame_symbols;
                    frame_out_symbols = expected_frame_symbols;
                }
            } else {
                d_waiting_for_input = true;
                break;
            }
        }

        // add tags
        if (d_tag_offset + expected_frame_symbols <= nitems_written(0) + write_index) {

            DTL_LOG_DEBUG("add tags: offset={}, "
                          "frame_in_bytes={}, "
                          "read_index={}, write_index={}, expected_frame={}, payload={}",
                          d_tag_offset,
                          frame_in_bytes,
                          read_index,
                          write_index,
                          expected_frame_symbols,
                          frame_payload);

            // Add TSB length tag - frame payload length in symbols
            add_item_tag(0,
                         d_tag_offset,
                         d_packet_len_tag,
                         pmt::from_long(expected_frame_symbols));
            // Add transported payload tag - number of payload bytes carried by the frame (including CRC)
            add_item_tag(0,
                         d_tag_offset,
                         payload_length_key(),
                         pmt::from_long(frame_payload + d_crc.get_crc_len()));
            // Add constellation tag - constellation used for the frame
            add_item_tag(0,
                         d_tag_offset,
                         get_constellation_tag_key(),
                         pmt::from_long(static_cast<int>(cnst)));
            d_tag_offset += expected_frame_symbols;
            d_frame_store.store(frame_payload, d_frame_count & 0xFFF, reinterpret_cast<char*>(&d_frame_buffer[0]));
            ++d_frame_count;
            pmt::pmt_t monitor_msg = pmt::make_dict();
            monitor_msg = pmt::dict_add(monitor_msg,
                                    FRAME_COUNT_KEY,
                                    pmt::from_long(d_frame_count));
            message_port_pub(MONITOR_PORT, monitor_msg);
        }
    }

    DTL_LOG_DEBUG("work: consumed={}, produced={}, frame_count={}", read_index, write_index, d_frame_count);
    consume_each(read_index);
    return write_index;
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
