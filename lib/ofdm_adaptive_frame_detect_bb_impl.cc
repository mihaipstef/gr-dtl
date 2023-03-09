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

static const int CONSECUTIVE_SYNCED_FRAMES_TH = 3;
static const int MAX_CONSECUTIVE_MISSING_CORRECTION = 5;


ofdm_adaptive_frame_detect_bb::sptr ofdm_adaptive_frame_detect_bb::make(int frame_len)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_detect_bb_impl>(frame_len);
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
      d_missing_count(0),
      d_correction_count(0),
      d_acc_error(0),
      d_trigger_counter(0),
      d_synced(0),
      d_in_sync(false)
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
    DTL_LOG_DEBUG("fix_sync begin: len={}, last_trigger={}, frame_len={}",
                  len,
                  last_trigger_index,
                  d_frame_len);
    for (int i = 0, index = d_remainder; i < len; ++i, ++index) {
        if (out[i]) {
            int frame_len_detected = i - last_trigger_index;
            int diff = frame_len_detected - d_frame_len;
            index = 0;

            // Count 1-sample errors detected
            if (d_acc_error) {
                ++d_trigger_counter;
            }

            int inst_error = abs(diff);
            // Accumulate 1-sample errors
            if (inst_error == 1) {
                d_acc_error += diff;
            }

            // Count conseccutive synced frames
            if (inst_error == 0 || inst_error == 1) {
                if (d_synced < CONSECUTIVE_SYNCED_FRAMES_TH) {
                    ++d_synced;
                } else {
                    d_in_sync = true;
                }
            } else {
                d_synced = 0;
                d_in_sync = false;
            }

            DTL_LOG_DEBUG("trigger found: buffer_begin={}, error={}, acc_error={}, "
                          "last_trigger={}, synced={}",
                          !trigger_found,
                          diff,
                          d_acc_error,
                          last_trigger_index,
                          d_synced);

            if (d_acc_error > 1 || (inst_error > 1 && inst_error < 10)) {
                if (last_trigger_index + d_frame_len < len) {
                    out[i] = 0;
                    last_trigger_index += d_frame_len;
                    out[last_trigger_index] = 1;
                    i = last_trigger_index;
                    d_acc_error = 0;
                    ++d_correction_count;
                } else {
                    last_trigger_index = i;
                }
            } else {
                last_trigger_index = i;
            }

            d_remainder = len - last_trigger_index;

            trigger_found = true;
            // Reset accumulated error after a given numbers of triggers
            if (d_trigger_counter > 10) {
                d_acc_error = 0;
            }
        }
        // If trigger not found at expected position generate one
        else if (d_in_sync && (index > d_frame_len + 10 ||
                               (index > d_frame_len + 1 && i == len - 1))) {
            DTL_LOG_DEBUG(
                "fix_sync correct: last_trigger={}, index={}", last_trigger_index, index);
            last_trigger_index += d_frame_len;
            out[last_trigger_index] = 1;
            i = last_trigger_index;
            index = 0;
            trigger_found = true;
            d_remainder = len - last_trigger_index;
            ++d_missing_count;
            if (--d_synced <
                CONSECUTIVE_SYNCED_FRAMES_TH - MAX_CONSECUTIVE_MISSING_CORRECTION) {
                d_in_sync = false;
                d_synced = 0;
            }
        }
    }

    if (!trigger_found) {
        d_remainder += len;
    }
    DTL_LOG_DEBUG("trigger correction counters: missing_triggers={}, trigger_correct={}",
                  d_missing_count,
                  d_correction_count);
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
