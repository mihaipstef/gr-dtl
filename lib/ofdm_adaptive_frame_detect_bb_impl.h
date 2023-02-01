/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_frame_detect_bb.h>

namespace gr {
namespace dtl {


class ofdm_adaptive_frame_detect_bb_impl : virtual public ofdm_adaptive_frame_detect_bb
{
private:
    int d_frame_len;

public:
    static const pmt::pmt_t header_port();

    ofdm_adaptive_frame_detect_bb_impl(int frame_len);
    ~ofdm_adaptive_frame_detect_bb_impl();

    void fill_gap(const char *in, char *out, int len);
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_IMPL_H */
