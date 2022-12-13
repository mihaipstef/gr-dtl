/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/dtl/ofdm_equalizer_adaptive.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

#include <algorithm>
#include <functional>

namespace gr {
namespace dtl {


using namespace gr::digital;



ofdm_equalizer_adaptive::sptr
ofdm_equalizer_adaptive::make(int fft_len,
                               const std::vector<constellation_type_t>& constellations,
                               const std::vector<std::vector<int>>& occupied_carriers,
                               const std::vector<std::vector<int>>& pilot_carriers,
                               const std::vector<std::vector<gr_complex>>& pilot_symbols,
                               int symbols_skipped,
                               float alpha,
                               bool input_is_shifted,
                               bool enable_soft_output)
{
    return ofdm_equalizer_adaptive::sptr(
        new ofdm_equalizer_adaptive(fft_len,
                                     constellations,
                                     occupied_carriers,
                                     pilot_carriers,
                                     pilot_symbols,
                                     symbols_skipped,
                                     alpha,
                                     input_is_shifted,
                                     enable_soft_output));
}


ofdm_equalizer_adaptive::ofdm_equalizer_adaptive(
    int fft_len,
    const std::vector<constellation_type_t>& constellations,
    const std::vector<std::vector<int>>& occupied_carriers,
    const std::vector<std::vector<int>>& pilot_carriers,
    const std::vector<std::vector<gr_complex>>& pilot_symbols,
    int symbols_skipped,
    float alpha,
    bool input_is_shifted,
    bool enable_soft_output)
    : ofdm_equalizer_1d_pilots(fft_len,
                               occupied_carriers,
                               pilot_carriers,
                               pilot_symbols,
                               symbols_skipped,
                               input_is_shifted),
      d_alpha(alpha),
      d_enable_soft_output(enable_soft_output)
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


ofdm_equalizer_adaptive::~ofdm_equalizer_adaptive() {}


void ofdm_equalizer_adaptive::equalize(gr_complex* frame,
                                        int n_sym,
                                        const std::vector<gr_complex>& initial_taps,
                                        const std::vector<tag_t>& tags)
{
    if (!initial_taps.empty()) {
        d_channel_state = initial_taps;
    }
    gr_complex sym_eq, sym_est;
    bool enable_soft_output = d_enable_soft_output;

    auto it = std::find_if(tags.begin(), tags.end(), [](auto& t) {
        return t.key == pmt::string_to_symbol("frame_constellation");
    });

    if (it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }

    constellation_type_t constellation_type_tag_value = get_constellation_type(tags);

    if (d_constellations.find(constellation_type_tag_value) ==
        d_constellations.end()) {
        throw std::invalid_argument("Unknown constellation");
    }

    constellation_sptr constellation = d_constellations[constellation_type_tag_value];

    for (int i = 0; i < n_sym; i++)
    {
        for (int k = 0; k < d_fft_len; k++) {
            if (!d_occupied_carriers[k]) {
                continue;
            }
            if (!d_pilot_carriers.empty() && d_pilot_carriers[d_pilot_carr_set][k]) {
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
}

} // namespace dtl
} /* namespace gr */
