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

ofdm_adaptive_frame_detect_bb::sptr ofdm_adaptive_frame_detect_bb::make(int frame_len)
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
      d_frame_len(frame_len),
      d_remainder(0),
      d_gaps_count(0),
      d_correction_count(0),
      d_acc_error(0),
      d_trigger_counter(0)
{
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_frame_detect_bb_impl::~ofdm_adaptive_frame_detect_bb_impl() {}


void ofdm_adaptive_frame_detect_bb_impl::forecast(int noutput_items,
                                                  gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = d_frame_len;
}


void ofdm_adaptive_frame_detect_bb_impl::fix_sync(const char* in, char* out, int len)
{
    int last_trigger_index = -d_remainder;

    memcpy(out, in, len);
    bool trigger_found = false;
    for (int i = 0; i < len; ++i) {
        if (out[i]) {
            int frame_len_detected = i - last_trigger_index;
            int diff = frame_len_detected - d_frame_len;
            int inst_error = abs(diff);

            // Count triggers id 1 sample error detected
            if (d_acc_error) {
                ++d_trigger_counter;
            }

            // If there is a gap between 2 triggers ...
            if (abs(inst_error - d_frame_len) < 10) {
                // ... fill the gap.
                last_trigger_index += d_frame_len;
                out[last_trigger_index] = 1;
                i = last_trigger_index;
                ++d_gaps_count;
            // otherwise try to correct the error.
            } else {
                // Accumulate 1 sample errors
                if (inst_error == 1) {
                    d_acc_error += diff;
                }
                if (d_acc_error > 1 || (inst_error > 1 && inst_error < 10)) {
                    if (last_trigger_index + d_frame_len < len) {
                        out[i] = 0;
                        last_trigger_index += d_frame_len;
                        out[last_trigger_index] = 1;
                        //i = last_trigger_index;
                        d_acc_error = 0;
                        ++d_correction_count;
                    } else {
                        last_trigger_index = i;
                    }
                } else {
                    last_trigger_index = i;
                }
            }
            d_remainder = len - last_trigger_index;
            DTL_LOG_DEBUG("start_buffer={} ,gaps detected={}, triggers corrected={}, "
                          "error={}, acc_error={}",
                          !trigger_found,
                          d_gaps_count,
                          d_correction_count,
                          diff,
                          d_acc_error);
            trigger_found = true;
            // Reset accumulated error after a given numbers of triggers
            if (d_trigger_counter > 10) {
                d_acc_error = 0;
            }
        }
    }
    if (!trigger_found) {
        d_remainder += len;
    }
}

int ofdm_adaptive_frame_detect_bb_impl::work(int noutput_items,
                                             gr_vector_const_void_star& input_items,
                                             gr_vector_void_star& output_items)
{
    auto in = static_cast<const char*>(input_items[0]);
    auto out = static_cast<char*>(output_items[0]);

    fix_sync(in, out, noutput_items);

    return noutput_items;
}

} /* namespace dtl */
} /* namespace gr */
