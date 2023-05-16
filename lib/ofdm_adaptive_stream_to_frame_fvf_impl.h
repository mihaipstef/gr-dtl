/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_stream_to_frame_fvf.h>

namespace gr {
namespace dtl {

class ofdm_adaptive_stream_to_frame_fvf_impl : public ofdm_adaptive_stream_to_frame_fvf
{
private:
    int d_frame_capacity;
    pmt::pmt_t d_len_key;



public:
    ofdm_adaptive_stream_to_frame_fvf_impl(int frame_capacity, const std::string& len_key);
    ~ofdm_adaptive_stream_to_frame_fvf_impl();

    // Where all the action really happens
    int general_work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_IMPL_H */
