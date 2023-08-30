/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_equalizer.h>
#include <gnuradio/dtl/ofdm_adaptive_feedback_decision.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief Modifies gr::digital::ofdm_frame_equalizer to pass the tags to equalize
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_frame_equalizer_vcvc
    : virtual public ::gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_frame_equalizer_vcvc> sptr;

    static sptr make(::gr::dtl::ofdm_adaptive_equalizer_base::sptr equalizer,
                     ofdm_adaptive_feedback_decision_base::sptr feedback_decision,
                     int cp_len,
                     const std::string& len_tag_key = "len_key",
                     const std::string& frame_no_key = "frame_no_key",
                     bool propagate_channel_state = false,
                     bool propagate_feedback_tags = false);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_H */
