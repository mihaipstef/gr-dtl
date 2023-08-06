/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_constellation_soft_cf : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_constellation_soft_cf> sptr;

    static sptr make(const std::vector<constellation_type_t>& constellations, const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_SOFT_CF_H */
