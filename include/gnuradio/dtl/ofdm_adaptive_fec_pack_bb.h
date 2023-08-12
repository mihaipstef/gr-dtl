/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_fec_pack_bb : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_fec_pack_bb> sptr;

    static sptr make(const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_H */
