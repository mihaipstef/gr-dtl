/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_packet_header.h>

#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>

#include <algorithm>

#include "logger.h"
#include <cassert>

namespace gr {
namespace dtl {

using namespace gr::digital;

INIT_DTL_LOGGER("ofdm_adaptive_packet_header");

ofdm_adaptive_packet_header::sptr
ofdm_adaptive_packet_header::make(const std::vector<std::vector<int>>& occupied_carriers,
                                  int header_syms,
                                  int payload_syms,
                                  const std::string& len_tag_key,
                                  const std::string& frame_len_tag_key,
                                  const std::string& num_tag_key,
                                  int bits_per_header_sym,
                                  bool scramble_header)
{
    return ofdm_adaptive_packet_header::sptr(
        new ofdm_adaptive_packet_header(occupied_carriers,
                                        header_syms,
                                        payload_syms,
                                        len_tag_key,
                                        frame_len_tag_key,
                                        num_tag_key,
                                        bits_per_header_sym,
                                        scramble_header));
}


ofdm_adaptive_packet_header::ofdm_adaptive_packet_header(
    const std::vector<std::vector<int>>& occupied_carriers,
    int header_syms,
    int payload_syms,
    const std::string& len_tag_key,
    const std::string& frame_len_tag_key,
    const std::string& num_tag_key,
    int bits_per_header_sym,
    bool scramble_header)
    : packet_header_ofdm(occupied_carriers,
                         header_syms,
                         len_tag_key,
                         frame_len_tag_key,
                         num_tag_key,
                         bits_per_header_sym,
                         0,
                         scramble_header),
      d_constellation_tag_key(get_constellation_tag_key()),
      d_constellation(constellation_type_t::QAM16),
      d_payload_syms(payload_syms)
{
}

ofdm_adaptive_packet_header::~ofdm_adaptive_packet_header() {}


bool ofdm_adaptive_packet_header::header_formatter(long packet_len,
                                                   unsigned char* out,
                                                   const std::vector<tag_t>& tags)
{

    auto it = find_constellation_tag(tags);
    if (it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }
    unsigned char constellation_type_tag_value = pmt::to_long(it->value) & 0xFF;
    it = find_tag(tags, payload_length_key());
    if (it == tags.end()) {
        throw std::invalid_argument("Missing payload length tag.");
    }
    size_t payload_length = pmt::to_long(it->value) & 0xFFF;

    DTL_LOG_DEBUG("header_formatter: cnst={}, payload_len={}, frame_no={}",
                  (int)constellation_type_tag_value,
                  payload_length,
                  d_header_number);

    memset(out, 0x00, d_header_len);
    int k = 0;
    // Data (payload) length
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((payload_length >> i) & d_mask);
    }
    // Packet number
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((d_header_number >> i) & d_mask);
    }
    // Constellation
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((constellation_type_tag_value >> i) & d_mask);
    }

    // Compute CRC and insert (Bit 32-39 - 8 bits)
    unsigned char buffer[] = { (unsigned char)(payload_length & 0xFF),
                               (unsigned char)(payload_length >> 8),
                               (unsigned char)(d_header_number & 0xFF),
                               (unsigned char)(d_header_number >> 8),
                               (unsigned char)constellation_type_tag_value };

    unsigned char crc = d_crc_impl.compute(buffer, sizeof(buffer));

    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((crc >> i) & d_mask);
    }

    // Incement packet number
    d_header_number++;
    d_header_number &= 0x0FFF;

    // Scramble
    for (int i = 0; i < d_header_len; i++) {
        out[i] ^= d_scramble_mask[i];
    }
    return true;
}

bool ofdm_adaptive_packet_header::header_parser(const unsigned char* in,
                                                std::vector<tag_t>& tags)
{

    size_t packet_len = 0;
    size_t packet_number = 0;
    unsigned char constellation_type = 0;

    int k = 0;
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        packet_len |= (((int)in[k]) & d_mask) << i;
    }
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        packet_number |= (((int)in[k]) & d_mask) << i;
    }
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        constellation_type |= (((int)in[k]) & d_mask) << i;
    }

    unsigned char buffer[] = { (unsigned char)(packet_len & 0xFF),
                               (unsigned char)(packet_len >> 8),
                               (unsigned char)(packet_number & 0xFF),
                               (unsigned char)(packet_number >> 8),
                               constellation_type };
    unsigned char crc_calcd = d_crc_impl.compute(buffer, sizeof(buffer));
    bool crc_ok = true;
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        if ((((int)in[k]) & d_mask) != (((int)crc_calcd >> i) & d_mask)) {
            crc_ok = false;
            break;
        }
    }

    // Update constellation only if CRC ok
    if (crc_ok && constellation_type &&
        constellation_type <= static_cast<unsigned char>(constellation_type_t::QAM16)) {
        d_constellation = static_cast<constellation_type_t>(constellation_type);
    }

    d_bits_per_payload_sym =
        compute_no_of_bits_per_symbol(static_cast<constellation_type_t>(d_constellation));

    if (d_bits_per_payload_sym == 0)
        return false;

    size_t no_of_symbols = packet_len * 8 / d_bits_per_payload_sym;
    if (packet_len * 8 % d_bits_per_payload_sym) {
        no_of_symbols++;
    }

    DTL_LOG_DEBUG("header_parser: cnst={}, payload_len={}, frame_no={}, crc_ok={}",
                  (int)d_constellation,
                  no_of_symbols,
                  packet_number,
                  crc_ok);

    // Add tags
    tag_t tag;
    tag.key = d_len_tag_key;
    tag.value = pmt::from_long(no_of_symbols);
    tags.push_back(tag);
    tag.key = d_num_tag_key;
    tag.value = pmt::from_long(packet_number);
    tags.push_back(tag);
    tag.key = get_constellation_tag_key();
    tag.value = pmt::from_long(static_cast<int>(d_constellation));
    tags.push_back(tag);
    tag.key = d_frame_len_tag_key;
    tag.value = pmt::from_long(d_payload_syms);
    tags.push_back(tag);
    return true;
}

} /* namespace dtl */
} /* namespace gr */
