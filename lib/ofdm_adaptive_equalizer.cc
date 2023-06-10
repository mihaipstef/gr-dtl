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
                              bool input_is_shifted)
{
    return ofdm_adaptive_equalizer::sptr(new ofdm_adaptive_equalizer(fft_len,
                                                                     constellations,
                                                                     snr_est,
                                                                     occupied_carriers,
                                                                     pilot_carriers,
                                                                     pilot_symbols,
                                                                     symbols_skipped,
                                                                     alpha,
                                                                     input_is_shifted));
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
    // Load all available pilot symbol sets.
    // ofdm_equalizer_1d_pilots loads only one pilot symbol set / pilot carrier set though the allocator
    // use all available pilot sets.
    int fft_shift_width = 0;
    if (input_is_shifted) {
        fft_shift_width = fft_len / 2;
    }
    for (unsigned i = 0; i < pilot_symbols.size(); i++) {
        int pilot_car_set = i % pilot_carriers.size();
        for (unsigned k = 0; k < pilot_carriers[pilot_car_set].size(); k++) {
            int carr_index = pilot_carriers[pilot_car_set][k];
            if (pilot_carriers[pilot_car_set][k] < 0) {
                carr_index += fft_len;
            }
            d_pilot_symbols[i][(carr_index + fft_shift_width) % fft_len] =
                pilot_symbols[i][k];
        }
    }
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
    bool input_is_shifted)
    : ofdm_adaptive_equalizer_base(fft_len,
                                   occupied_carriers,
                                   pilot_carriers,
                                   pilot_symbols,
                                   symbols_skipped,
                                   input_is_shifted),
      d_alpha(alpha),
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
                                       gr_complex* frame_soft,
                                       int n_sym,
                                       const std::vector<gr_complex>& initial_taps,
                                       const std::vector<tag_t>& tags)
{
    if (!initial_taps.empty()) {
        d_channel_state = initial_taps;
    }
    gr_complex sym_eq, sym_est, pilot_eq;

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

    // Reset SNR estimator each frame
    d_snr_estimator->reset();
    int pilot_symbols_set = 0;
    for (int i = 0; i < n_sym; i++) {
        pilot_symbols_set = (d_symbols_skipped + i) % d_pilot_symbols.size();
        for (int k = 0; k < d_fft_len; k++) {
            bool is_pilot_carreier =
                !d_pilot_carriers.empty() && d_pilot_carriers[d_pilot_carr_set][k];
            if (!d_occupied_carriers[k] && !is_pilot_carreier) {
                continue;
            }
            if (is_pilot_carreier) {
                pilot_eq = frame[i * d_fft_len + k] / d_channel_state[k];
                // Update SNR estimation with each pilot
                d_snr_estimator->update(1, &pilot_eq);
                // Update channel state
                d_channel_state[k] = d_alpha * d_channel_state[k] +
                                     (1 - d_alpha) * frame[i * d_fft_len + k] /
                                         d_pilot_symbols[pilot_symbols_set][k];
                frame[i * d_fft_len + k] = d_pilot_symbols[pilot_symbols_set][k];
            } else {
                sym_eq = frame[i * d_fft_len + k] / d_channel_state[k];
                // NOTE: The `map_to_points` function will treat `sym_est` as an array
                // pointer.  This call is "safe" only for 1-dimension constellations.
                constellation->map_to_points(constellation->decision_maker(&sym_eq),
                                             &sym_est);
                d_channel_state[k] = d_alpha * d_channel_state[k] +
                                     (1 - d_alpha) * frame[i * d_fft_len + k] / sym_est;
                frame[i * d_fft_len + k] = sym_est;
                if (frame_soft) {
                    frame_soft[i * d_fft_len + k] = sym_eq;
                }
            }
        }
        if (!d_pilot_carriers.empty()) {
            d_pilot_carr_set = (d_pilot_carr_set + 1) % d_pilot_carriers.size();
        }
    }

    DTL_LOG_DEBUG("n_sym:{}, SNRest:{}, Constellation:{}, d_pilot_carr_set:{}",
                  n_sym,
                  get_snr(),
                  static_cast<int>(cnst_type),
                  d_pilot_carriers.empty());

}

void ofdm_adaptive_equalizer::equalize(gr_complex* frame,
                                       int n_sym,
                                       const std::vector<gr_complex>& initial_taps,
                                       const std::vector<tag_t>& tags)
{
    equalize(frame, nullptr, n_sym, initial_taps, tags);
}


double ofdm_adaptive_equalizer::get_snr() { return d_snr_estimator->snr(); }


} // namespace dtl
} /* namespace gr */
