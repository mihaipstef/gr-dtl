/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H

#include <gnuradio/digital/constellation.h>
#include <gnuradio/tags.h>

#include <cstddef>
#include <tuple>

namespace gr {
namespace dtl {

enum class constellation_type_t {
    UNKNOWN = 0,
    BPSK,
    QPSK,
    PSK8,
    QAM16,
};

std::size_t compute_no_of_bits_per_symbol(constellation_type_t constellation);

gr::digital::constellation_sptr determine_constellation(constellation_type_t constellation); 

constellation_type_t get_constellation_type(const std::vector<tag_t>& tags);

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H */
