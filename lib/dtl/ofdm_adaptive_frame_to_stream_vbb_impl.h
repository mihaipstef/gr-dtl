/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_frame_to_stream_vbb.h>

namespace gr {
namespace dtl {

class ofdm_adaptive_frame_to_stream_vbb_impl : public ofdm_adaptive_frame_to_stream_vbb
{
private:
    pmt::pmt_t d_len_key;
    int d_frame_capacity;

public:
    ofdm_adaptive_frame_to_stream_vbb_impl(int frame_capacity, std::string& len_key);
    ~ofdm_adaptive_frame_to_stream_vbb_impl();

    int fixed_rate_ninput_to_noutput(int ninput_items);
    int fixed_rate_noutput_to_ninput(int noutput_items);
    void forecast(int noutput_items, gr_vector_int& ninput_items_required);


    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_IMPL_H */
