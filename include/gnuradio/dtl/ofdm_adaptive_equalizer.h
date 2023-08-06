/* -*- c++ -*- */
/* Copyright 2012 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_DTL_OFDM_EQUALIZER_ADAPTIVE_H
#define INCLUDED_DTL_OFDM_EQUALIZER_ADAPTIVE_H

#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/mpsk_snr_est.h>
#include <gnuradio/digital/ofdm_equalizer_base.h>
#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_frame_snr.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>


namespace gr {
namespace dtl {

/*!
 * \brief Enhance the equalizer interface for Adaptive OFDM transmission.
 *
 * \ingroup dtl
 * \details
 *
 */
class ofdm_adaptive_equalizer_base : public gr::digital::ofdm_equalizer_1d_pilots
{
public:
    typedef std::shared_ptr<ofdm_adaptive_equalizer_base> sptr;

    ofdm_adaptive_equalizer_base(
        int fft_len,
        const std::vector<std::vector<int>>& occupied_carriers,
        const std::vector<std::vector<int>>& pilot_carriers,
        const std::vector<std::vector<gr_complex>>& pilot_symbols,
        int symbols_skipped,
        bool input_is_shifted);

    virtual double get_snr() = 0;

    virtual double get_noise() = 0;

    virtual void
    equalize(gr_complex* frame,
             int n_sym,
             const std::vector<gr_complex>& initial_taps = std::vector<gr_complex>(),
             const std::vector<tag_t>& tags = std::vector<gr::tag_t>()) = 0;

    virtual void
    equalize(gr_complex* frame,
             gr_complex* frame_soft,
             int n_sym,
             const std::vector<gr_complex>& initial_taps = std::vector<gr_complex>(),
             const std::vector<tag_t>& tags = std::vector<gr::tag_t>()) = 0;
};


/*!
 * \brief Modify gr::digital::ofdm_equalizer_simpledfe to allow different constellation
 * for each frame. \ingroup dtl
 *
 * \details
 *
 */
class DTL_API ofdm_adaptive_equalizer : public ofdm_adaptive_equalizer_base
{
public:
    typedef std::shared_ptr<ofdm_adaptive_equalizer> sptr;

    ofdm_adaptive_equalizer(int fft_len,
                            const std::vector<constellation_type_t>& constellations,
                            const std::shared_ptr<ofdm_adaptive_frame_snr_base> snr_est,
                            const std::vector<std::vector<int>>& occupied_carriers =
                                std::vector<std::vector<int>>(),
                            const std::vector<std::vector<int>>& pilot_carriers =
                                std::vector<std::vector<int>>(),
                            const std::vector<std::vector<gr_complex>>& pilot_symbols =
                                std::vector<std::vector<gr_complex>>(),
                            int symbols_skipped = 0,
                            float alpha = 0.1,
                            bool input_is_shifted = true);

    ~ofdm_adaptive_equalizer() override;

    void equalize(gr_complex* frame,
                  int n_sym,
                  const std::vector<gr_complex>& initial_taps = std::vector<gr_complex>(),
                  const std::vector<gr::tag_t>& tags = std::vector<gr::tag_t>()) override;

    void equalize(gr_complex* frame,
                  gr_complex* frame_soft,
                  int n_sym,
                  const std::vector<gr_complex>& initial_taps = std::vector<gr_complex>(),
                  const std::vector<gr::tag_t>& tags = std::vector<gr::tag_t>()) override;

    double get_snr() override;

    double get_noise() override;

    static sptr make(int fft_len,
                     const std::vector<constellation_type_t>& constellations,
                     const std::shared_ptr<ofdm_adaptive_frame_snr_base> snr_est,
                     const std::vector<std::vector<int>>& occupied_carriers =
                         std::vector<std::vector<int>>(),
                     const std::vector<std::vector<int>>& pilot_carriers =
                         std::vector<std::vector<int>>(),
                     const std::vector<std::vector<gr_complex>>& pilot_symbols =
                         std::vector<std::vector<gr_complex>>(),
                     int symbols_skipped = 0,
                     float alpha = 0.1,
                     bool input_is_shifted = true);

private:
    constellation_dictionary_t d_constellations;
    float d_alpha;
    std::vector<gr_complex> d_pilots;
    std::shared_ptr<ofdm_adaptive_frame_snr_base> d_snr_estimator;
};

} /* namespace dtl */
} /* namespace gr */

#endif /* INCLUDED_DTL_OFDM_EQUALIZER_ADAPTIVE_H */
