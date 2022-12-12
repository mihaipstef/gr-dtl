/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

#include <algorithm>
#include <functional>
#include <map>

namespace gr {
namespace dtl {


using namespace gr::digital;


template <class T>
struct constellation_helper {
    static std::function<constellation_sptr()> constructor()
    {
        return []() { return T::make(); };
    }
};


std::map<constellation_type_t, std::function<constellation_sptr()>>
    constellation_constructor = {
        { constellation_type_t::BPSK,
          constellation_helper<constellation_bpsk>::constructor() },
        { constellation_type_t::QPSK,
          constellation_helper<constellation_qpsk>::constructor() },
        { constellation_type_t::PSK8,
          constellation_helper<constellation_8psk>::constructor() },
        { constellation_type_t::QAM16,
          constellation_helper<constellation_16qam>::constructor() },
    };


std::map<constellation_type_t, std::size_t> _bits_per_symbol = {
    { constellation_type_t::UNKNOWN, 0 },
    { constellation_type_t::BPSK, 1 },
    { constellation_type_t::QPSK, 2 },
    { constellation_type_t::PSK8, 3 },
    { constellation_type_t::QAM16, 4 }
};


std::size_t DTL_API compute_no_of_bits_per_symbol(constellation_type_t constellation_type)
{
    if (auto it = _bits_per_symbol.find(constellation_type);
        it != _bits_per_symbol.end()) {
        return it->second;
    }
    return 0;
}

constellation_sptr DTL_API
determine_constellation(constellation_type_t constellation_type)
{
    if (auto it = constellation_constructor.find(constellation_type);
        it != constellation_constructor.end()) {
        return it->second();
    }
    return nullptr;
}

constellation_type_t DTL_API
get_constellation_type(const std::vector<tag_t>& tags)
{
    auto it = std::find_if(tags.begin(), tags.end(), [](auto& t) {
        return t.key == pmt::string_to_symbol("frame_constellation");
    });

    if (it == tags.end()) {
        return constellation_type_t::UNKNOWN;
    }

    return static_cast<constellation_type_t>(pmt::to_long(it->value));
}

} /* namespace dtl */
} /* namespace gr */