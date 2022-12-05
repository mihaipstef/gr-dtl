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

#include <gnuradio/digital/api.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/ofdm_equalizer_base.h>

#include <map>

namespace gr {
namespace dtl {

enum class constellation_type_t {
    BPSK = 0,
    QPSK,
    PSK8,
    QAM16,
};


/*!
 * \brief Modify gr::digital::ofdm_equalizer_simpledfe to allow different constellation for each frame.
 * \ingroup dtl
 *
 * \details
 * 
 */
class DIGITAL_API ofdm_equalizer_adaptive : public gr::digital::ofdm_equalizer_1d_pilots
{
public:
    typedef std::shared_ptr<ofdm_equalizer_adaptive> sptr;

    ofdm_equalizer_adaptive(int fft_len,
                             const std::vector<constellation_type_t>& constellations,
                             const std::vector<std::vector<int>>& occupied_carriers =
                                 std::vector<std::vector<int>>(),
                             const std::vector<std::vector<int>>& pilot_carriers =
                                 std::vector<std::vector<int>>(),
                             const std::vector<std::vector<gr_complex>>& pilot_symbols =
                                 std::vector<std::vector<gr_complex>>(),
                             int symbols_skipped = 0,
                             float alpha = 0.1,
                             bool input_is_shifted = true,
                             bool enable_soft_output = false);

    ~ofdm_equalizer_adaptive() override;

    void equalize(gr_complex* frame,
                  int n_sym,
                  const std::vector<gr_complex>& initial_taps = std::vector<gr_complex>(),
                  const std::vector<tag_t>& tags = std::vector<tag_t>()) override;


    static sptr make(int fft_len,
                     const std::vector<constellation_type_t>& constellations,
                     const std::vector<std::vector<int>>& occupied_carriers =
                         std::vector<std::vector<int>>(),
                     const std::vector<std::vector<int>>& pilot_carriers =
                         std::vector<std::vector<int>>(),
                     const std::vector<std::vector<gr_complex>>& pilot_symbols =
                         std::vector<std::vector<gr_complex>>(),
                     int symbols_skipped = 0,
                     float alpha = 0.1,
                     bool input_is_shifted = true,
                     bool enable_soft_output = false);

private:

    typedef std::map<constellation_type_t, gr::digital::constellation_sptr> constellation_dictionary_t;

    constellation_dictionary_t d_constellations;
    float d_alpha;
    bool d_enable_soft_output;
};

} /* namespace dtl */
} /* namespace gr */

#endif /* INCLUDED_DTL_OFDM_EQUALIZER_ADAPTIVE_H */
