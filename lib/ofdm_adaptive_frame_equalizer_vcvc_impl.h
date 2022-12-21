/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_frame_equalizer_vcvc.h>

namespace gr {
namespace dtl {

class ofdm_adaptive_frame_equalizer_vcvc_impl : public ofdm_adaptive_frame_equalizer_vcvc
{
private:
    const int d_fft_len;
    const int d_cp_len;
    gr::dtl::ofdm_adaptive_equalizer_base::sptr d_eq;
    bool d_propagate_channel_state;
    const int d_fixed_frame_len;
    std::vector<gr_complex> d_channel_state;
    const pmt::pmt_t d_decision_feedback_port;
    ofdm_adaptive_feedback_decision_base::sptr d_decision_feedback;
    bool d_propagate_feedback_tags;

protected:
    void parse_length_tags(const std::vector<std::vector<tag_t>>& tags,
                           gr_vector_int& n_input_items_reqd) override;

public:
    ofdm_adaptive_frame_equalizer_vcvc_impl(
        gr::dtl::ofdm_adaptive_equalizer_base::sptr equalizer,
        ofdm_adaptive_feedback_decision_base::sptr feedback_decision,
        int cp_len,
        const std::string& len_tag_key,
        bool propagate_channel_state,
        bool propagate_feedback_tags,
        int fixed_frame_len);
    ~ofdm_adaptive_frame_equalizer_vcvc_impl() override;

    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_IMPL_H */
