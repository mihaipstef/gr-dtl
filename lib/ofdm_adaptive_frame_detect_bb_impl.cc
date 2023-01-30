/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_frame_detect_bb_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_frame_detect_bb");

ofdm_adaptive_frame_detect_bb::sptr
ofdm_adaptive_frame_detect_bb::make(int frame_len, int detect_counter)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_detect_bb_impl>(frame_len,
                                                                         detect_counter);
}

const pmt::pmt_t ofdm_adaptive_frame_detect_bb_impl::header_port()
{
    static const pmt::pmt_t header_port = pmt::mp("header_port");
    return header_port;
}

/*
 * The private constructor
 */
ofdm_adaptive_frame_detect_bb_impl::ofdm_adaptive_frame_detect_bb_impl(int frame_len,
                                                                       int detect_counter)
    : d_frame_len(frame_len),
      d_state(frame_detect_state_t::NOT_SYNCED),
      d_length_counter(0),
      d_remaining(0),
      d_detect_counter(detect_counter),
      gr::sync_block(
          "ofdm_adaptive_frame_detect_bb",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(char)),
          gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(char)))
{
    message_port_register_in(header_port());
    set_msg_handler(header_port(), [this](pmt::pmt_t msg) { this->parse_header(msg); });
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_frame_detect_bb_impl::~ofdm_adaptive_frame_detect_bb_impl() {}


void ofdm_adaptive_frame_detect_bb_impl::parse_header(pmt::pmt_t header_data)
{
    if (pmt::is_dict(header_data)) {
        pmt::pmt_t header_items(pmt::dict_items(header_data));
        if (!pmt::is_null(header_items)) {
            // Don't change the state
            DTL_LOG_DEBUG("parse_header: state={}",
                  (d_state == frame_detect_state_t::SYNCED) ? "SYNCED" : "NOT_SYNCED");
            return;
        }
    }
    // If header failed switch to NOT_SYNCED
    d_state = frame_detect_state_t::NOT_SYNCED;
    DTL_LOG_DEBUG("parse_header: state={}",
                  (d_state == frame_detect_state_t::SYNCED) ? "SYNCED" : "NOT_SYNCED");
}


void ofdm_adaptive_frame_detect_bb_impl::generate_trigger(char* out, int len)
{
    DTL_LOG_DEBUG("generate_trigger: len={}, d_remaining={}", len, d_remaining);
    memset(out, 0, len);
    int index = 0;
    while (index < len) {
        out[index] = 1;
        index += d_frame_len;
    }
    // Set remaining samples after the last trigger
    d_remaining = d_frame_len - (index - len);
}


int ofdm_adaptive_frame_detect_bb_impl::detect_sync(const char* in, int len)
{
    int last_trigger_index = -1;
    for (int i = 0; i < len; ++i) {
        if (in[i]) {
            last_trigger_index = i;
            if ((d_remaining + i) % d_frame_len == 0) {
                ++d_length_counter;
            } else {
                d_length_counter = 0;
            }
            if (d_length_counter == d_detect_counter) {
                DTL_LOG_DEBUG("detect_sync: found");
                return i;
            }
        }
    }
    // Set remaining samples if no trigger was detected
    if (last_trigger_index >= 0) {
        d_remaining = len - last_trigger_index;
    }
    return -1;
}


int ofdm_adaptive_frame_detect_bb_impl::work(int noutput_items,
                                             gr_vector_const_void_star& input_items,
                                             gr_vector_void_star& output_items)
{
    auto in = static_cast<const char*>(input_items[0]);
    auto out = static_cast<char*>(output_items[0]);

    switch (d_state) {
    case frame_detect_state_t::NOT_SYNCED: {
        // detect in the buffer where we get in the sync mode
        int offset = detect_sync(in, noutput_items);
        if (offset >= 0) {
            // copy mem before offset
            memcpy(out, in, offset);
            // generate the rest of the triggers
            generate_trigger(&out[offset], noutput_items - offset);
            // switch to sync mode
            d_state = frame_detect_state_t::SYNCED;
        } else {
            memcpy(out, in, noutput_items);
        }
    } break;
    case frame_detect_state_t::SYNCED: {
        // generate triggers
        int offset = d_frame_len - d_remaining;
        if (offset < noutput_items) {
            memset(out, 0, offset);
            generate_trigger(&out[offset], noutput_items-offset);
        } else {
            memset(out, 0, noutput_items);
            d_remaining += noutput_items;
        }
    }break;
    default:
        throw std::runtime_error("wrong state");
    }
    DTL_LOG_DEBUG("work: state={}, noutput_items={}",
                  (d_state == frame_detect_state_t::SYNCED) ? "SYNCED" : "NOT_SYNCED",
                  noutput_items);
    return noutput_items;
}

} /* namespace dtl */
} /* namespace gr */
