/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_DECODER_CB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_DECODER_CB_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief Decodes received symbols using the constellation passed with the tags.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_constellation_decoder_cb
    : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_constellation_decoder_cb> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of
     * dtl::ofdm_adaptive_constellation_decoder_cb.
     */
    static sptr make(const std::vector<constellation_type_t>& constellations,
                     const std::string& tsb_tag_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CONSTELLATION_DECODER_CB_H */
