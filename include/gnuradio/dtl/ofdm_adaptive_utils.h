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
#include <pmt/pmt.h>

#include <cstddef>
#include <map>
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

typedef std::map<constellation_type_t, gr::digital::constellation_sptr>
    constellation_dictionary_t;

gr::digital::constellation_sptr get_constellation(constellation_type_t constellation_type);

std::size_t get_bits_per_symbol(constellation_type_t constellation);

std::pair<constellation_type_t, std::size_t>
get_max_bps(const std::vector<constellation_type_t>& cnsts);

gr::digital::constellation_sptr create_constellation(constellation_type_t constellation);

constellation_type_t find_constellation_type(const std::vector<tag_t>& tags);

constellation_type_t get_constellation_type(const tag_t& tag);

std::vector<tag_t>::const_iterator find_constellation_tag(const std::vector<tag_t>& tags);

std::vector<tag_t>::const_iterator find_tag(const std::vector<tag_t>& tags,
                                            const pmt::pmt_t& key);

pmt::pmt_t get_constellation_tag_key();

pmt::pmt_t estimated_snr_tag_key();

pmt::pmt_t noise_tag_key();

pmt::pmt_t feedback_constellation_key();

pmt::pmt_t payload_length_key();

pmt::pmt_t fec_key();

pmt::pmt_t fec_feedback_key();

pmt::pmt_t fec_tb_key();

pmt::pmt_t fec_offset_key();

pmt::pmt_t fec_tb_payload_key();

pmt::pmt_t fec_tb_len_key();

pmt::pmt_t fec_tb_index_key();


} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_UTILS_H */
