/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

#include <map>

namespace gr {
namespace dtl {


std::map<constellation_type_t, std::size_t> _bits_per_symbol = {
    { constellation_type_t::BPSK, 1 }, { constellation_type_t::QPSK, 2 },
    { constellation_type_t::PSK8, 3 }, { constellation_type_t::QAM16, 4 }
};


std::size_t DTL_API compute_no_of_bits_per_symbol(constellation_type_t constellation)
{
    if (auto it = _bits_per_symbol.find(constellation); it != _bits_per_symbol.end()) {
        return it->second;
    }
    return 0;
}


} /* namespace dtl */
} /* namespace gr */