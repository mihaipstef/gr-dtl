/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_constellation_soft_cf.h>
#include <gnuradio/digital/constellation.h>


namespace gr {
namespace dtl {

class ofdm_adaptive_constellation_soft_cf_impl
    : public ofdm_adaptive_constellation_soft_cf
{
private:
    constellation_dictionary_t d_constellations;
    pmt::pmt_t d_len_key;
    uint64_t d_tag_offset;
    digital::constellation_sptr d_constellation;

public:
    ofdm_adaptive_constellation_soft_cf_impl(const std::vector<constellation_type_t>& constellations, const std::string& len_key);
    ~ofdm_adaptive_constellation_soft_cf_impl();

    void forecast(int noutput_items,
                                   gr_vector_int& ninput_items_required) override;
    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_IMPL_H */
