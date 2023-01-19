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

    /*!
     * \param equalizer The equalizer object that will do the actual work
     * \param cp_len Length of the cyclic prefix in samples (required to correct the
     * frequency offset) \param tsb_key TSB key \param propagate_channel_state If true,
     * the channel state after the last symbol will be added to the first symbol as a tag
     * \param fixed_frame_len Set if the frame length is fixed. When this value is given,
     *                        the TSB tag key can be left empty, but it is useful even
     *                        when using tagged streams at the input.
     */
    static sptr make(::gr::dtl::ofdm_adaptive_equalizer_base::sptr equalizer,
                     ofdm_adaptive_feedback_decision_base::sptr feedback_decision,
                     int cp_len,
                     const std::string& tsb_key = "frame_len",
                     bool propagate_channel_state = false,
                     bool propagate_feedback_tags = false,
                     int fixed_frame_len = 0);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_EQUALIZER_VCVC_H */
