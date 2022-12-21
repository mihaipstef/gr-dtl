/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_feedback_decision.h>

#include <utility>
#include <vector>

namespace gr {
namespace dtl {


#define INSERT_DECISION_ENTRY(table, lower_snr_th, higher_snr_th, constellation_type, fec_scheme) \
    table.push_back(std::make_pair( \
        std::make_pair<double, double>(lower_snr_th, higher_snr_th), \
        ofdm_adaptive_feedback_t(constellation_type, fec_scheme) \
    ))

ofdm_adaptive_feedback_decision::sptr ofdm_adaptive_feedback_decision::make()
{
    return std::make_shared<ofdm_adaptive_feedback_decision>();
}

ofdm_adaptive_feedback_decision::ofdm_adaptive_feedback_decision()
{
    // Construct the lookup table for decision
    INSERT_DECISION_ENTRY(feedback_lut, 5, 10, constellation_type_t::BPSK, 0);
    INSERT_DECISION_ENTRY(feedback_lut, 10, 15, constellation_type_t::QPSK, 0);
    INSERT_DECISION_ENTRY(feedback_lut, 15, 20, constellation_type_t::PSK8, 0);
    INSERT_DECISION_ENTRY(feedback_lut, 20, 100, constellation_type_t::QAM16, 0);
}

ofdm_adaptive_feedback_decision::~ofdm_adaptive_feedback_decision()
{
}


ofdm_adaptive_feedback_t ofdm_adaptive_feedback_decision::get_feedback(double estimated_snr)
{
    for (auto& lut_entry: feedback_lut)
    {
        if (estimated_snr >= lut_entry.first.first && estimated_snr < lut_entry.first.second) {
            return lut_entry.second;
        }
    }
    return ofdm_adaptive_feedback_t(constellation_type_t::UNKNOWN, 0);
}


} /* namespace dtl */
} /* namespace gr */