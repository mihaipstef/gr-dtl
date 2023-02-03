/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <gnuradio/dtl/ofdm_adaptive_feedback_decision.h>
#include <limits>
#include "logger.h"
#include <utility>
#include <vector>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_feedback_decision");

ofdm_adaptive_feedback_decision_base::~ofdm_adaptive_feedback_decision_base() {}

ofdm_adaptive_feedback_decision::sptr ofdm_adaptive_feedback_decision::make(double hysterisis, int decision_th)
{
    return std::make_shared<ofdm_adaptive_feedback_decision>(hysterisis, decision_th);
}

ofdm_adaptive_feedback_decision::ofdm_adaptive_feedback_decision(double hysterisis, int decision_th)
    : d_hyteresis(hysterisis), d_decision_th(decision_th), d_decision_counter(0)
{
    // Construct the lookup table for decision
    feedback_lut.push_back(
        std::make_pair(std::numeric_limits<double>::min(), constellation_type_t::BPSK));
    feedback_lut.push_back(
        std::make_pair(15, constellation_type_t::QPSK));
    feedback_lut.push_back(
        std::make_pair(18, constellation_type_t::PSK8));
    feedback_lut.push_back(
        std::make_pair(21, constellation_type_t::QAM16));

}

ofdm_adaptive_feedback_decision::~ofdm_adaptive_feedback_decision() {}

ofdm_adaptive_feedback_t
ofdm_adaptive_feedback_decision::get_feedback(constellation_type_t current_cnst,
                                              double estimated_snr)
{
    DTL_LOG_DEBUG("Get feedback for snr={}", estimated_snr);
    auto it = std::find_if(feedback_lut.begin(), feedback_lut.end(), [&](auto& t) {
        return t.second == current_cnst;
    });

    if (it == feedback_lut.end()) {
        return static_cast<ofdm_adaptive_feedback_t>(current_cnst);
    }

    if (estimated_snr < (it->first - d_hyteresis)) {
        if (is_new_decision((--it)->second)) {
            return static_cast<ofdm_adaptive_feedback_t>(d_decision);
        }
    } else if (++it != feedback_lut.end()) {
        if (is_new_decision(it->second)) {
            return static_cast<ofdm_adaptive_feedback_t>(d_decision);
        }
    }
    return static_cast<ofdm_adaptive_feedback_t>(current_cnst);
}


bool ofdm_adaptive_feedback_decision::is_new_decision(constellation_type_t cnst) {
    if (cnst != d_decision) {
        d_decision_counter = 0;
        d_decision = cnst;
    } else {
        if (++d_decision_counter >= d_decision_th) {
            d_decision_counter = 0;
            return true;
        }
    }
    return false;
}

} /* namespace dtl */
} /* namespace gr */