/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_SNR_ESTIMATOR_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_SNR_ESTIMATOR_H

#include <gnuradio/gr_complex.h>

namespace gr {
namespace dtl {


class ofdm_adaptive_frame_snr_base
{
public:
    virtual int update(int noutput_items, const gr_complex* input) = 0;
    virtual void reset() = 0;
    virtual double snr() = 0;
    virtual double noise() = 0;
};


template <class T>
class ofdm_adaptive_frame_snr : public ofdm_adaptive_frame_snr_base
{
public:
    ofdm_adaptive_frame_snr(double alpha) {
        d_snr_estimator = std::make_shared<T>(alpha);
    }

    virtual void reset() override { d_snr_estimator = std::make_shared<T>(d_snr_estimator->alpha()); }
    virtual int update(int noutput_items, const gr_complex* input) override
    {
        return d_snr_estimator->update(noutput_items, input);
    }

    virtual double snr() override { return d_snr_estimator->snr(); }
    virtual double noise() override { return d_snr_estimator->noise(); }

protected:
    std::shared_ptr<T> d_snr_estimator;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_SNR_ESTIMATOR_H */