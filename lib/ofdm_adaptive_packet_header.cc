/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_packet_header.h>

#include "logger.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cassert>


namespace gr {
namespace dtl {

using namespace gr::digital;
using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_packet_header")

static const int FEC_HEADER_LEN = 7;

ofdm_adaptive_packet_header::sptr
ofdm_adaptive_packet_header::make(const std::vector<std::vector<int>>& occupied_carriers,
                                  int header_syms,
                                  int payload_syms,
                                  const std::string& len_tag_key,
                                  const std::string& frame_len_tag_key,
                                  const std::string& num_tag_key,
                                  int bits_per_header_sym,
                                  bool scramble_header,
                                  bool has_fec)
{
    return ofdm_adaptive_packet_header::sptr(
        new ofdm_adaptive_packet_header(occupied_carriers,
                                        header_syms,
                                        payload_syms,
                                        len_tag_key,
                                        frame_len_tag_key,
                                        num_tag_key,
                                        bits_per_header_sym,
                                        scramble_header,
                                        has_fec));
}


ofdm_adaptive_packet_header::ofdm_adaptive_packet_header(
    const std::vector<std::vector<int>>& occupied_carriers,
    int header_syms,
    int payload_syms,
    const std::string& len_tag_key,
    const std::string& frame_len_tag_key,
    const std::string& num_tag_key,
    int bits_per_header_sym,
    bool scramble_header,
    bool has_fec)
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
      d_payload_syms(payload_syms),
      d_has_fec(has_fec),
      d_crc(16, 0x1021, 0xFFFF, 0, false, true)
{
}

ofdm_adaptive_packet_header::~ofdm_adaptive_packet_header() {}


int ofdm_adaptive_packet_header::add_header_field(unsigned char* buf,
                                                  int offset,
                                                  unsigned long long val,
                                                  int n_bits)
{
    int k = offset;
    for (int i = 0; i < n_bits && k < d_header_len; i += d_bits_per_byte, k++) {
        buf[k] = (unsigned char)((val >> i) & d_mask);
    }
    return k;
}

int ofdm_adaptive_packet_header::add_fec_header(const std::vector<tag_t>& tags,
                                                unsigned char* out,
                                                int first_pos,
                                                vector<unsigned char>& crc_buf)
{

    static std::map<pmt::pmt_t, tuple<int, int>> _fec_tags_to_header = {
        { fec_codeword_key(),
          make_tuple(0, 16) }, // add codeword numbers (bit 32-47: 16 bits)
        { fec_offset_key(),
          make_tuple(16, 16) },           // add first codeword index (bit 48-63: 16 bits)
        { fec_key(), make_tuple(32, 8) }, // add FEC scheme (bit 64-71: 8 bits)
        { fec_padding_key(), make_tuple(40, 16) }, // add FEC transport block padding(bit 72-87: 16 bits)

    };

    int fec_header_start = crc_buf.size();
    crc_buf.resize(fec_header_start + FEC_HEADER_LEN);
    int next_pos = first_pos;
    for (auto& tag : tags) {
        auto it = _fec_tags_to_header.find(tag.key);
        if (it != _fec_tags_to_header.end()) {
            int offset = get<0>(it->second);
            int len = get<1>(it->second);
            int val = pmt::to_long(tag.value);
            int j = offset / d_bits_per_byte; // ONLY WORKS FOR d_bits_per_byte=0,1,4,8
            add_header_field(out, first_pos + j, val, len);
            next_pos += len / d_bits_per_byte;
            // Asumes byte alignement
            assert(offset % 8 == 0);
            for (int i = 0; i < len/8; ++i) {
                crc_buf[fec_header_start+offset/8+i] = (val >> i*8) & 0xFF;
            }
        }
    }
    return next_pos;
}

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
    // Data (payload) length (bit 0-11: 12 bits)
    k = add_header_field(out, k, payload_length, 12);
    // Frame number (bit 12-23: 12 bits)
    k = add_header_field(out, k, d_header_number, 12);
    // Constellation (bit 24-31: 8 bits)
    k = add_header_field(out, k, constellation_type_tag_value, 8);

    vector<unsigned char> buffer_crc = { (unsigned char)(payload_length & 0xFF),
                                         (unsigned char)(payload_length >> 8),
                                         (unsigned char)(d_header_number & 0xFF),
                                         (unsigned char)(d_header_number >> 8),
                                         (unsigned char)constellation_type_tag_value };

    if (d_has_fec) {
        // Add FEC tags to header (6 bytes) and return next position for CRC
        k = add_fec_header(tags, out, k, buffer_crc);
    }
    // Compute CRC and insert (Bit 32-47 (short header) / bit 88-103 (long header): 16
    // bits)
    unsigned crc = d_crc.compute(&buffer_crc[0], buffer_crc.size());
    k = add_header_field(out, k, crc, 16);

    // Incement packet number
    d_header_number++;
    d_header_number &= 0x0FFF;

    // Scramble
    for (int i = 0; i < d_header_len; i++) {
        out[i] ^= d_scramble_mask[i];
    }
    return true;
}


int ofdm_adaptive_packet_header::parse_fec_header(const unsigned char* in,
                                                  int first_pos,
                                                  std::vector<tag_t>& tags,
                                                  vector<unsigned char>& crc_buf)
{
    static vector<tuple<int, int, pmt::pmt_t>> fec_header_to_tags = {
        make_tuple(0, 16, fec_codeword_key()),
        make_tuple(16, 16, fec_offset_key()),
        make_tuple(32, 8, fec_key()),
        make_tuple(40, 16, fec_padding_key()),
    };
    int fec_header_start = crc_buf.size();
    crc_buf.resize(fec_header_start + FEC_HEADER_LEN);
    int k = first_pos;
    for (auto& h : fec_header_to_tags) {
        int val = 0;
        int offset = get<0>(h);
        int len = get<1>(h);
        for (int i = 0; i < len && k < d_header_len; i += d_bits_per_byte, k++) {
            val |= (((int)in[k]) & d_mask) << i;
        }
        // Add to CRC buffer
        assert(offset % 8 == 0); // Asumes byte alignement
        int n = (len % 8) ? len / 8 + 1: len / 8;
        for (int i = 0; i < n; ++i) {
            crc_buf[fec_header_start + offset / 8 + i] = (val >> i * 8) & 0xFF;
        }
        // Add tags
        tag_t tag;
        tag.key = get<2>(h);
        tag.value = pmt::from_long(val);
        tags.push_back(tag);
    }
    return k;
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
    vector<unsigned char> buffer = { (unsigned char)(packet_len & 0xFF),
                               (unsigned char)(packet_len >> 8),
                               (unsigned char)(packet_number & 0xFF),
                               (unsigned char)(packet_number >> 8),
                               constellation_type };


    if (d_has_fec) {
        k = parse_fec_header(in, k, tags, buffer);
    }


    unsigned crc_calcd = d_crc.compute(&buffer[0], buffer.size());
    for (int i = 0; i < 16 && k < d_header_len; i += d_bits_per_byte, k++) {
        if ((((int)in[k]) & d_mask) != (((int)crc_calcd >> i) & d_mask)) {
            DTL_LOG_DEBUG("header_parser: crc=failed");
            return false;
        }
    }

    // Update constellation only if CRC ok
    if (constellation_type &&
        constellation_type <= static_cast<unsigned char>(constellation_type_t::QAM16)) {
        d_constellation = static_cast<constellation_type_t>(constellation_type);
    }

    d_bits_per_payload_sym =
        get_bits_per_symbol(static_cast<constellation_type_t>(d_constellation));

    if (d_bits_per_payload_sym == 0)
        return false;

    size_t no_of_symbols = packet_len * 8 / d_bits_per_payload_sym;
    if (packet_len * 8 % d_bits_per_payload_sym) {
        no_of_symbols++;
    }

    DTL_LOG_DEBUG("header_parser: cnst={}, payload_len={}, frame_no={}, crc=ok",
                  (int)d_constellation,
                  no_of_symbols,
                  packet_number);

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
