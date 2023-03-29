/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "constellation.h"
#include "logger.h"
#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <algorithm>
#include <functional>
#include <map>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_utils")

using namespace gr::digital;

static const pmt::pmt_t CONSTELLATION_TAG_KEY =
    pmt::string_to_symbol("constellation_tag_key");
static const pmt::pmt_t ESTIMATED_SNR_TAG_KEY =
    pmt::string_to_symbol("estimated_snr_tag_key");
static const pmt::pmt_t FEEDBACK_CONSTELLATION_KEY =
    pmt::string_to_symbol("feedback_constellation_key");
static const pmt::pmt_t FEEDBACK_FEC_KEY = pmt::string_to_symbol("feedback_fec_key");
static const pmt::pmt_t PAYLOAD_LENGTH_KEY = pmt::string_to_symbol("payload_length_key");


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
          constellation_helper<constellation_qpsk_normalized>::constructor() },
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


gr::digital::constellation_sptr DTL_API get_constellation(constellation_type_t constellation_type)
{
    static constellation_dictionary_t all_constellations;
    if (auto it = all_constellations.find(constellation_type);
        it != all_constellations.end()) {
            return it->second;
    }
    auto new_cnst = create_constellation(constellation_type);
    if (new_cnst) {
        all_constellations[constellation_type] = new_cnst;
    }
    return new_cnst;
}

std::size_t DTL_API get_bits_per_symbol(constellation_type_t constellation_type)
{
    if (auto it = _bits_per_symbol.find(constellation_type);
        it != _bits_per_symbol.end()) {
        return it->second;
    }
    return 0;
}

std::pair<constellation_type_t, std::size_t>
    DTL_API get_max_bps(const std::vector<constellation_type_t>& cnsts)
{
    auto max_bps = std::make_pair<constellation_type_t, std::size_t>(
        constellation_type_t::UNKNOWN, 0);
    for (auto cnst : cnsts) {
        auto it_bps = _bits_per_symbol.find(cnst);
        if (it_bps != _bits_per_symbol.end() && it_bps->second > max_bps.second) {
            max_bps = *it_bps;
        }
    }
    return max_bps;
}

constellation_sptr DTL_API create_constellation(constellation_type_t constellation_type)
{
    if (auto it = constellation_constructor.find(constellation_type);
        it != constellation_constructor.end()) {
        return it->second();
    }
    return nullptr;
}

constellation_type_t DTL_API get_constellation_type(const tag_t& tag)
{
    return static_cast<constellation_type_t>(pmt::to_long(tag.value));
}

constellation_type_t DTL_API find_constellation_type(const std::vector<tag_t>& tags)
{
    auto it = find_constellation_tag(tags);
    if (it == tags.end()) {
        return constellation_type_t::UNKNOWN;
    }
    return get_constellation_type(*it);
}

std::vector<tag_t>::const_iterator DTL_API
find_constellation_tag(const std::vector<tag_t>& tags)
{
    auto it = std::find_if(
        tags.begin(), tags.end(), [](auto& t) { return t.key == CONSTELLATION_TAG_KEY; });
    return it;
}

std::vector<tag_t>::const_iterator DTL_API find_tag(const std::vector<tag_t>& tags,
                                                    const pmt::pmt_t& key)
{
    auto it =
        std::find_if(tags.begin(), tags.end(), [&](auto& t) { return t.key == key; });
    return it;
}


pmt::pmt_t DTL_API get_constellation_tag_key() { return CONSTELLATION_TAG_KEY; }

pmt::pmt_t DTL_API estimated_snr_tag_key() { return ESTIMATED_SNR_TAG_KEY; }

pmt::pmt_t DTL_API feedback_constellation_key() { return FEEDBACK_CONSTELLATION_KEY; }

pmt::pmt_t DTL_API feedback_fec_key() { return FEEDBACK_FEC_KEY; }

pmt::pmt_t DTL_API payload_length_key() { return PAYLOAD_LENGTH_KEY; }

} /* namespace dtl */
} /* namespace gr */