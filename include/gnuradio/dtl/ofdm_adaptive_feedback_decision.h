/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_DECISION_DEFAULT_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_DECISION_DEFAULT_H


#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>


namespace gr {
namespace dtl {


typedef std::pair<constellation_type_t, unsigned char> ofdm_adaptive_feedback_t;
typedef int mcs_id_t;

class DTL_API ofdm_adaptive_feedback_decision_base
    : public std::enable_shared_from_this<ofdm_adaptive_feedback_decision_base>
{
public:
    typedef std::shared_ptr<ofdm_adaptive_feedback_decision_base> sptr;

    virtual ofdm_adaptive_feedback_t get_feedback(double estimated_snr) = 0;

    virtual ~ofdm_adaptive_feedback_decision_base();
};


/*!
 * \brief
 *
 * \ingroup DTL
 *
 * \details
 *
 */
class DTL_API ofdm_adaptive_feedback_decision
    : public ofdm_adaptive_feedback_decision_base
{
public:
    typedef std::shared_ptr<ofdm_adaptive_feedback_decision> sptr;

    static sptr make(double hysterisis,
                     int decision_th,
                     const std::vector<std::pair<double, ofdm_adaptive_feedback_t>>& lut,
                     mcs_id_t initial_mcs = 0);

    ofdm_adaptive_feedback_decision(
        double hysterisis,
        int decision_counter,
        const std::vector<std::pair<double, ofdm_adaptive_feedback_t>>& lut,
        mcs_id_t initial_mcs = 0);

    ~ofdm_adaptive_feedback_decision() override;

    virtual ofdm_adaptive_feedback_t get_feedback(double estimated_snr) override;

private:
    void update_decision(mcs_id_t mcs_id);

    std::vector<std::pair<double, ofdm_adaptive_feedback_t>> d_feedback_lut;
    double d_hyteresis;
    int d_decision_th;
    int d_decision_counter;
    mcs_id_t d_last_decision;
    mcs_id_t d_new_decision;
};

} /* namespace dtl */
} /* namespace gr */

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_DECISION_DEFAULT_H */
