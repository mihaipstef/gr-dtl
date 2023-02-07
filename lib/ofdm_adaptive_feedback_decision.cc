/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include <gnuradio/dtl/ofdm_adaptive_feedback_decision.h>
#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_feedback_decision");

ofdm_adaptive_feedback_decision_base::~ofdm_adaptive_feedback_decision_base() {}

ofdm_adaptive_feedback_decision::sptr
ofdm_adaptive_feedback_decision::make(double hysterisis, int decision_th)
{
    return std::make_shared<ofdm_adaptive_feedback_decision>(hysterisis, decision_th);
}

ofdm_adaptive_feedback_decision::ofdm_adaptive_feedback_decision(double hysterisis,
                                                                 int decision_th)
    : d_hyteresis(hysterisis),
      d_decision_th(decision_th),
      d_decision_counter(0),
      d_last_decision(constellation_type_t::UNKNOWN),
      d_new_decision(constellation_type_t::UNKNOWN)
{
    // Construct the lookup table for decision
    feedback_lut.push_back(
        std::make_pair(std::numeric_limits<double>::min(), constellation_type_t::BPSK));
    feedback_lut.push_back(std::make_pair(15, constellation_type_t::QPSK));
    feedback_lut.push_back(std::make_pair(18, constellation_type_t::PSK8));
    feedback_lut.push_back(std::make_pair(21, constellation_type_t::QAM16));
}

ofdm_adaptive_feedback_decision::~ofdm_adaptive_feedback_decision() {}

ofdm_adaptive_feedback_t
ofdm_adaptive_feedback_decision::get_feedback(constellation_type_t current_cnst,
                                              double estimated_snr)
{
    DTL_LOG_DEBUG(
        "Get feedback for snr={}, last_decision={}, new_decision={}, counter={}",
        estimated_snr,
        (int)d_last_decision,
        (int)d_new_decision,
        d_decision_counter);

    if (d_last_decision == constellation_type_t::UNKNOWN) {
        d_last_decision = current_cnst;
    }

    auto it = std::find_if(feedback_lut.begin(), feedback_lut.end(), [&](auto& t) {
        return t.second == d_last_decision;
    });

    if (it == feedback_lut.end()) {
        // Something terribly wrong must have happen, so fallback to current constellation
        d_last_decision = current_cnst;
        return static_cast<ofdm_adaptive_feedback_t>(current_cnst);
    }

    if (estimated_snr < it->first) {
        update_decision((--it)->second);
    } else if (++it != feedback_lut.end() && estimated_snr > (it->first + d_hyteresis)) {
        update_decision(it->second);
    }
    return static_cast<ofdm_adaptive_feedback_t>(d_last_decision);
}


void ofdm_adaptive_feedback_decision::update_decision(constellation_type_t cnst)
{
    if (cnst != d_new_decision) {
        d_decision_counter = 0;
        d_new_decision = cnst;
    } else {
        if (++d_decision_counter >= d_decision_th) {
            d_decision_counter = 0;
            d_last_decision = d_new_decision;
        }
    }
}

} /* namespace dtl */
} /* namespace gr */