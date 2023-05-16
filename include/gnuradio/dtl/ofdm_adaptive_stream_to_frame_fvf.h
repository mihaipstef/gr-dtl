/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/block.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_stream_to_frame_fvf : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_stream_to_frame_fvf> sptr;

    static sptr make(int frame_capacity, const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_STREAM_TO_FRAME_FVF_H */
