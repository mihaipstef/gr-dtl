/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_constellation_metric_vcvf.h>

namespace gr {
namespace dtl {

class ofdm_adaptive_constellation_metric_vcvf_impl
    : public ofdm_adaptive_constellation_metric_vcvf
{
private:
    unsigned int d_fft_len;
    std::vector<bool> d_subcarriers;
    std::map<constellation_type_t, double> d_min_dist;

public:
    ofdm_adaptive_constellation_metric_vcvf_impl(
        unsigned int fft_len,
        const std::vector<unsigned>& subcarriers,
        const std::vector<constellation_type_t>& constellations,
        const std::string& len_key);
    ~ofdm_adaptive_constellation_metric_vcvf_impl();

    float get_cnst_min_distance(constellation_type_t cnst) override;

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_IMPL_H */
