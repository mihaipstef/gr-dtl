/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_fec_decoder : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_fec_decoder> sptr;
    static sptr make(const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_H */
