/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

namespace gr {
namespace dtl {

/*!
 * \brief Tag the input stream according to the feedback received.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_frame_bb : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_frame_bb> sptr;

    static sptr make(const std::string& len_tag_key,
                     const std::vector<constellation_type_t>& constellations,
                     size_t frame_len,
                     size_t n_payload_carriers);

    virtual void set_constellation(constellation_type_t constellation) = 0;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_H */