/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_REPACK_BITS_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_REPACK_BITS_BB_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/endianness.h>
#include <gnuradio/tagged_stream_block.h>


namespace gr {
namespace dtl {

/*!
 * \brief Extends gr::blocks::repack_bits_bb to use constellation type tag
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_repack_bits_bb : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_repack_bits_bb> sptr;

    /*!
     * \param tsb_tag_key If not empty, this is the key for the length tag.
     * \param endianness The endianness of the output data stream (LSB or MSB).
     */
    static sptr make(const std::string& tsb_tag_key = "",
                     bool unpack = true,
                     endianness_t endianness = GR_LSB_FIRST);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_REPACK_BITS_BB_H */
