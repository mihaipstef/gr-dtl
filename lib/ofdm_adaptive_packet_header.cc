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
#include "tag_utilities.h"


namespace gr {
namespace dtl {

using namespace gr::digital;
using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_packet_header")

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
      d_crc(16, 0x1021, 0xFFFF, 0, false, true),
      d_crc_len(16)
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


void ofdm_adaptive_packet_header::pack_crc(const unsigned char* header_bits, vector<unsigned char>& crc_buf)
{
    int len = (d_header_len - d_crc_len) / 8;
    if ((d_header_len - d_crc_len) % 8) {
        ++len;
    }
    crc_buf.resize(len, 0);
    for (int i=0; i<len; ++i) {
        for (int j=0; j<8 && i*len+j < d_header_len; j++) {
            crc_buf[i] = (crc_buf[i] << 1) | (header_bits[i*8+j] & 0x01);
       }
    }
}


int ofdm_adaptive_packet_header::add_fec_header(const std::vector<tag_t>& tags,
                                                unsigned char* out,
                                                int first_pos)
{

    static std::map<pmt::pmt_t, tuple<int, int>> _fec_tags_to_header = {
        { fec_tb_key(),
          make_tuple(0, 12) },                          // TB id numbers (bit 32-43: 12 bits)
        { fec_feedback_key(),
          make_tuple(12, 4) },                           // FEC feedback scheme (bit 44-47: 4 bits)
        { fec_offset_key(),
          make_tuple(16, 12) },                         // TB offset (bit 48-59: 12 bits)
        { fec_key(), make_tuple(28, 4) },               // FEC scheme (bit 60-63: 4 bits)
        { fec_tb_payload_key(), make_tuple(32, 16) },   // FEC transport block payload (bit 64-80: 16 bits)

    };

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
            DTL_LOG_DEBUG("fec_formatter: tag={}, val={}", pmt::symbol_to_string(tag.key), val);
        }
    }
    return next_pos;
}

bool ofdm_adaptive_packet_header::header_formatter(long packet_len,
                                                   unsigned char* out,
                                                   const std::vector<tag_t>& tags)
{
    std::map<pmt::pmt_t, tag_t> my_tags = get_tags(
        tags,
        get_constellation_tag_key(),
        payload_length_key(),
        feedback_constellation_key()
    );

    if (my_tags.size() != 3) {
        //throw std::invalid_argument
        DTL_LOG_DEBUG("Missing tags.");
    }

    unsigned char cnst = pmt::to_long(my_tags[get_constellation_tag_key()].value) & 0xF;
    unsigned char feedback_cnst = pmt::to_long(my_tags[feedback_constellation_key()].value) & 0xF;
    size_t payload_length = pmt::to_long(my_tags[payload_length_key()].value) & 0xFFF;

    DTL_LOG_DEBUG("header_formatter: cnst={}, payload_len={}, frame_no={}",
                  (int)cnst,
                  payload_length,
                  d_header_number);

    memset(out, 0x00, d_header_len);
    int k = 0;
    // Data (payload) length (bit 0-11: 12 bits)
    k = add_header_field(out, k, payload_length, 12);
    // Frame number (bit 12-23: 12 bits)
    k = add_header_field(out, k, d_header_number, 12);
    // Constellation (bit 24-27: 4 bits)
    k = add_header_field(out, k, cnst, 4);
    // Reverse feedback (bit 28-21: 4 bits)
    k = add_header_field(out, k, feedback_cnst, 4);

    if (d_has_fec) {
        // Add FEC tags to header (6 bytes) and return next position for CRC
        k = add_fec_header(tags, out, k);
    }

    // Compute CRC and insert (Bit 32-47 (short header) / bit 88-103 (long header): 16
    // bits)
    vector<unsigned char> buffer_crc;
    pack_crc(out, buffer_crc);
    unsigned crc = d_crc.compute(&buffer_crc[0], buffer_crc.size());
    k = add_header_field(out, k, crc, 16);

    // Incement packet number
    d_header_number++;
    d_header_number &= 0x0FFF;

    // Scramble
    for (int i = 0; i < d_header_len; i++) {
        out[i] ^= d_scramble_mask[i];
    }
    DTL_LOG_DEBUG("header_formatter: out");
    return true;
}


int ofdm_adaptive_packet_header::parse_fec_header(const unsigned char* in,
                                                  int first_pos,
                                                  std::vector<tag_t>& tags)
{
    static vector<tuple<int, int, pmt::pmt_t>> fec_header_to_tags = {
        make_tuple(0, 12, fec_tb_key()),
        make_tuple(12, 4, fec_feedback_key()),
        make_tuple(16, 12, fec_offset_key()),
        make_tuple(28, 4, fec_key()),
        make_tuple(32, 16, fec_tb_payload_key()),
    };

    int k = first_pos;
    for (auto& h : fec_header_to_tags) {
        int val = 0;
        int len = get<1>(h);
        for (int i = 0; i < len && k < d_header_len; i += d_bits_per_byte, k++) {
            val |= (((int)in[k]) & d_mask) << i;
        }
        // Add tags
        tag_t tag;
        tag.key = get<2>(h);
        tag.value = pmt::from_long(val);
        tags.push_back(tag);
        DTL_LOG_DEBUG("fec_parser: tag={}, val={}", pmt::symbol_to_string(tag.key), val);
    }
    return k;
}

bool ofdm_adaptive_packet_header::header_parser(const unsigned char* in,
                                                std::vector<tag_t>& tags)
{

    size_t payload_len = 0;
    size_t frame_no = 0;
    unsigned char cnst = 0;
    unsigned char feedback_cnst = 0;

    int k = 0;
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        payload_len |= (((int)in[k]) & d_mask) << i;
    }
    for (int i = 0; i < 12 && k < d_header_len; i += d_bits_per_byte, k++) {
        frame_no |= (((int)in[k]) & d_mask) << i;
    }
    for (int i = 0; i < 4 && k < d_header_len; i += d_bits_per_byte, k++) {
        cnst |= (((int)in[k]) & d_mask) << i;
    }
    for (int i = 0; i < 4 && k < d_header_len; i += d_bits_per_byte, k++) {
        feedback_cnst |= (((int)in[k]) & d_mask) << i;
    }

    if (d_has_fec) {
        k = parse_fec_header(in, k, tags);
    }

    vector<unsigned char> buffer;
    pack_crc(in, buffer);

    unsigned crc_calcd = d_crc.compute(&buffer[0], buffer.size());
    for (int i = 0; i < 16 && k < d_header_len; i += d_bits_per_byte, k++) {
        if ((((int)in[k]) & d_mask) != (((int)crc_calcd >> i) & d_mask)) {
            DTL_LOG_DEBUG("header_parser: crc=failed");
            return false;
        }
    }

    // Update constellation only if CRC ok
    if (cnst &&
        cnst <= static_cast<unsigned char>(constellation_type_t::QAM16)) {
        d_constellation = static_cast<constellation_type_t>(cnst);
    }

    d_bits_per_payload_sym =
        get_bits_per_symbol(static_cast<constellation_type_t>(d_constellation));

    if (d_bits_per_payload_sym == 0)
        return false;

    size_t no_of_symbols = payload_len * 8 / d_bits_per_payload_sym;
    if (payload_len * 8 % d_bits_per_payload_sym) {
        no_of_symbols++;
    }

    DTL_LOG_DEBUG("header_parser: cnst={}, payload_len={}, frame_no={}, crc=ok",
                  (int)d_constellation,
                  no_of_symbols,
                  frame_no);

    // Add tags
    tag_t tag;
    tag.key = payload_length_key();
    tag.value = pmt::from_long(payload_len);
    tags.push_back(tag);
    tag.key = d_len_tag_key;
    tag.value = pmt::from_long(no_of_symbols);
    tags.push_back(tag);
    tag.key = d_num_tag_key;
    tag.value = pmt::from_long(frame_no);
    tags.push_back(tag);
    tag.key = get_constellation_tag_key();
    tag.value = pmt::from_long(static_cast<int>(d_constellation));
    tags.push_back(tag);
    tag.key = d_frame_len_tag_key;
    tag.value = pmt::from_long(d_payload_syms);
    tags.push_back(tag);
    tag.key = feedback_constellation_key();
    tag.value = pmt::from_long(feedback_cnst);
    tags.push_back(tag);
    return true;
}

} /* namespace dtl */
} /* namespace gr */
