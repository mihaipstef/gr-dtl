/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_H

#include <gnuradio/sync_block.h>
#include <gnuradio/dtl/api.h>

namespace gr {
namespace dtl {

/*!
 * \brief Corrects the frame start trigger based on frame length.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_frame_detect_bb : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_frame_detect_bb> sptr;

    static sptr make(int frame_len);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_DETECT_BB_H */
