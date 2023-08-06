/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>

namespace gr {
namespace dtl {

/*!
 * \brief Converts frames to stream.
 * \ingroup DTL
 *
 */
class DTL_API ofdm_adaptive_frame_to_stream_vbb : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_frame_to_stream_vbb> sptr;

    //ofdm_adaptive_frame_to_stream_vbb(int frame_capacity, std::string& len_key);

    static sptr make(int frame_capacity, std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_TO_STREAM_VBB_H */
