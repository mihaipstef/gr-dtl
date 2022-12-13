/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_chunks_to_symbols_bc : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_chunks_to_symbols_bc> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of
     * dtl::ofdm_adaptive_chunks_to_symbols_bc.
     */
    static sptr make(
        const std::vector<constellation_type_t>& constellations,
        const std::string& tsb_tag_key
    );
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_H */
