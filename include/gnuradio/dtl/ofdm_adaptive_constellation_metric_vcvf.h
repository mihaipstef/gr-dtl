/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief Compute channel quality metric based on received constellation.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_constellation_metric_vcvf
    : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_constellation_metric_vcvf> sptr;

    static sptr make(unsigned int fft_len,
                     const std::vector<unsigned>& subcarriers,
                     const std::vector<constellation_type_t>& constellations,
                     const std::string& len_key);

    virtual float get_cnst_min_distance(constellation_type_t cnst) = 0;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_METRIC_VCVF_H */
