/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/testbed/logger.h>
#include <gnuradio/dtl/ofdm_adaptive_feedback_decision.h>
#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>
#include <vector>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_feedback_decision");

ofdm_adaptive_feedback_decision_base::~ofdm_adaptive_feedback_decision_base() {}

ofdm_adaptive_feedback_decision::sptr ofdm_adaptive_feedback_decision::make(
    double hysterisis,
    int decision_th,
    const std::vector<std::pair<double, ofdm_adaptive_feedback_t>>& lut,
    mcs_id_t initial_mcs)
{
    return std::make_shared<ofdm_adaptive_feedback_decision>(
        hysterisis, decision_th, lut, initial_mcs);
}

ofdm_adaptive_feedback_decision::ofdm_adaptive_feedback_decision(
    double hysterisis,
    int decision_th,
    const std::vector<std::pair<double, ofdm_adaptive_feedback_t>>& lut,
    mcs_id_t initial_mcs)
    : d_feedback_lut(lut),
      d_hyteresis(hysterisis),
      d_decision_th(decision_th),
      d_decision_counter(0),
      d_last_decision(initial_mcs),
      d_new_decision(initial_mcs)
{
    // Construct the lookup table for decision
    if (d_feedback_lut.size() == 0) {
        throw std::runtime_error("Feedback lookup table is empty");
    }
    // SNR of the first entry must be minimum possible value
    d_feedback_lut[0].first = std::numeric_limits<double>::min();
}

ofdm_adaptive_feedback_decision::~ofdm_adaptive_feedback_decision() {}

ofdm_adaptive_feedback_t
ofdm_adaptive_feedback_decision::get_feedback(double estimated_snr)
{
    DTL_LOG_DEBUG(
        "Get feedback for snr={}, last_decision={}, new_decision={}, counter={}",
        estimated_snr,
        (int)d_last_decision,
        (int)d_new_decision,
        d_decision_counter);

    mcs_id_t current_mcs_id = d_last_decision;
    auto& mcs = d_feedback_lut[current_mcs_id];

    if (estimated_snr < mcs.first) {
        assert(current_mcs_id > 0);
        update_decision(current_mcs_id-1);
    } else if (current_mcs_id+1 < d_feedback_lut.size()) {
        auto& better_mcs = d_feedback_lut[current_mcs_id+1];
        if (estimated_snr > (better_mcs.first + d_hyteresis)) {
            update_decision(current_mcs_id+1);
        } else {
            d_decision_counter = 0;
        }
    } else {
        d_decision_counter = 0;
    }
    return d_feedback_lut[d_last_decision].second;
}


void ofdm_adaptive_feedback_decision::update_decision(mcs_id_t mcs_id)
{
    if (mcs_id != d_new_decision) {
        d_decision_counter = 0;
        d_new_decision = mcs_id;
    } else {
        if (++d_decision_counter >= d_decision_th) {
            d_decision_counter = 0;
            d_last_decision = d_new_decision;
        }
    }
}

} /* namespace dtl */
} /* namespace gr */