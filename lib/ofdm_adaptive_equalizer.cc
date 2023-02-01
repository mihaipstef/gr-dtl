/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logger.h"

#include <gnuradio/dtl/ofdm_adaptive_equalizer.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

#include <algorithm>
#include <functional>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_equalizer")

using namespace gr::digital;


ofdm_adaptive_equalizer::sptr
ofdm_adaptive_equalizer::make(int fft_len,
                              const std::vector<constellation_type_t>& constellations,
                              const std::shared_ptr<ofdm_adaptive_frame_snr_base> snr_est,
                              const std::vector<std::vector<int>>& occupied_carriers,
                              const std::vector<std::vector<int>>& pilot_carriers,
                              const std::vector<std::vector<gr_complex>>& pilot_symbols,
                              int symbols_skipped,
                              float alpha,
                              bool input_is_shifted,
                              bool enable_soft_output)
{
    return ofdm_adaptive_equalizer::sptr(new ofdm_adaptive_equalizer(fft_len,
                                                                     constellations,
                                                                     snr_est,
                                                                     occupied_carriers,
                                                                     pilot_carriers,
                                                                     pilot_symbols,
                                                                     symbols_skipped,
                                                                     alpha,
                                                                     input_is_shifted,
                                                                     enable_soft_output));
}


ofdm_adaptive_equalizer_base::ofdm_adaptive_equalizer_base(
    int fft_len,
    const std::vector<std::vector<int>>& occupied_carriers,
    const std::vector<std::vector<int>>& pilot_carriers,
    const std::vector<std::vector<gr_complex>>& pilot_symbols,
    int symbols_skipped,
    bool input_is_shifted)
    : ofdm_equalizer_1d_pilots(fft_len,
                               occupied_carriers,
                               pilot_carriers,
                               pilot_symbols,
                               symbols_skipped,
                               input_is_shifted)
{
}


ofdm_adaptive_equalizer::ofdm_adaptive_equalizer(
    int fft_len,
    const std::vector<constellation_type_t>& constellations,
    const std::shared_ptr<ofdm_adaptive_frame_snr_base> snr_est,
    const std::vector<std::vector<int>>& occupied_carriers,
    const std::vector<std::vector<int>>& pilot_carriers,
    const std::vector<std::vector<gr_complex>>& pilot_symbols,
    int symbols_skipped,
    float alpha,
    bool input_is_shifted,
    bool enable_soft_output)
    : ofdm_adaptive_equalizer_base(fft_len,
                                   occupied_carriers,
                                   pilot_carriers,
                                   pilot_symbols,
                                   symbols_skipped,
                                   input_is_shifted),
      d_alpha(alpha),
      d_enable_soft_output(enable_soft_output),
      d_snr_estimator(snr_est)
{
    // Populate constellation dictionary
    for (const auto& constellation_type : constellations) {
        auto constellation = create_constellation(constellation_type);
        if (constellation == nullptr) {
            throw std::invalid_argument("Unknown constellation");
        }
        d_constellations[constellation_type] = constellation;
    }
}


ofdm_adaptive_equalizer::~ofdm_adaptive_equalizer() {}


void ofdm_adaptive_equalizer::equalize(gr_complex* frame,
                                       int n_sym,
                                       const std::vector<gr_complex>& initial_taps,
                                       const std::vector<tag_t>& tags)
{
    if (!initial_taps.empty()) {
        d_channel_state = initial_taps;
    }
    gr_complex sym_eq, sym_est;
    bool enable_soft_output = d_enable_soft_output;

    auto cnst_tag_it = find_constellation_tag(tags);

    if (cnst_tag_it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }

    constellation_type_t cnst_type = get_constellation_type(*cnst_tag_it);

    if (d_constellations.find(cnst_type) == d_constellations.end()) {
        throw std::invalid_argument("Unknown constellation");
    }

    constellation_sptr constellation =
        d_constellations[get_constellation_type(*cnst_tag_it)];

    d_snr_estimator->reset();
    for (int i = 0; i < n_sym; i++) {
        for (int k = 0; k < d_fft_len; k++) {
            bool is_pilot_carreier = !d_pilot_carriers.empty() && d_pilot_carriers[d_pilot_carr_set][k];
             if (!d_occupied_carriers[k] && !is_pilot_carreier) {
                 continue;
             }
            if (is_pilot_carreier) {
                // Update SNR estimation with each pilot
                d_snr_estimator->update(1, &frame[i * d_fft_len + k]);
                // Update channel state
                d_channel_state[k] = d_alpha * d_channel_state[k] +
                                     (1 - d_alpha) * frame[i * d_fft_len + k] /
                                         d_pilot_symbols[d_pilot_carr_set][k];
                frame[i * d_fft_len + k] = d_pilot_symbols[d_pilot_carr_set][k];
            } else {
                sym_eq = frame[i * d_fft_len + k] / d_channel_state[k];
                // NOTE: The `map_to_points` function will treat `sym_est` as an array
                // pointer.  This call is "safe" only for 1-dimension constellations.
                constellation->map_to_points(constellation->decision_maker(&sym_eq),
                                             &sym_est);
                d_channel_state[k] = d_alpha * d_channel_state[k] +
                                     (1 - d_alpha) * frame[i * d_fft_len + k] / sym_est;
                frame[i * d_fft_len + k] = enable_soft_output ? sym_eq : sym_est;
            }
        }
        if (!d_pilot_carriers.empty()) {
            d_pilot_carr_set = (d_pilot_carr_set + 1) % d_pilot_carriers.size();
        }
    }
    DTL_LOG_DEBUG("n_sym:{}, SNRest:{}, Constellation:{}, d_pilot_carr_set:{}",
        n_sym, get_snr(), static_cast<int>(cnst_type), d_pilot_carriers.empty());
}


double ofdm_adaptive_equalizer::get_snr() { return d_snr_estimator->snr(); }


} // namespace dtl
} /* namespace gr */
