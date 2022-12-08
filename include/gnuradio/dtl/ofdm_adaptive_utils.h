/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H

#include <cstddef>

namespace gr {
namespace dtl {


enum class constellation_type_t {
    BPSK = 0,
    QPSK,
    PSK8,
    QAM16,
};


std::size_t compute_no_of_bits_per_symbol(constellation_type_t constellation);


} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H */
