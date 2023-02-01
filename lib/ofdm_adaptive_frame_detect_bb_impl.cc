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
ofdm_adaptive_frame_detect_bb::make(int frame_len)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_detect_bb_impl>(frame_len);
}

const pmt::pmt_t ofdm_adaptive_frame_detect_bb_impl::header_port()
{
    static const pmt::pmt_t header_port = pmt::mp("header_port");
    return header_port;
}

/*
 * The private constructor
 */
ofdm_adaptive_frame_detect_bb_impl::ofdm_adaptive_frame_detect_bb_impl(int frame_len)
    : gr::sync_block(
          "ofdm_adaptive_frame_detect_bb",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(char)),
          gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(char))),
        d_frame_len(frame_len)
{
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_frame_detect_bb_impl::~ofdm_adaptive_frame_detect_bb_impl() {}

void ofdm_adaptive_frame_detect_bb_impl::fill_gap(const char *in, char *out, int len) {
    int last_trigger_index = -1;
    memcpy(out, in, len);
    for (int i = 0; i < len; ++i) {
        if (in[i]) {
            int frame_len_detected = i - last_trigger_index;
            if (abs(frame_len_detected - 2 * d_frame_len) < 3) {
                out[last_trigger_index + d_frame_len] = 1;
            }
            last_trigger_index = i;
        }
    }
}

int ofdm_adaptive_frame_detect_bb_impl::work(int noutput_items,
                                             gr_vector_const_void_star& input_items,
                                             gr_vector_void_star& output_items)
{
    auto in = static_cast<const char*>(input_items[0]);
    auto out = static_cast<char*>(output_items[0]);

    fill_gap(in, out, noutput_items);

    return noutput_items;
}

} /* namespace dtl */
} /* namespace gr */
