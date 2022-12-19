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


namespace gr {
namespace dtl {

using namespace gr::digital;

INIT_DTL_LOGGER("ofdm_adaptive_packet_header");

ofdm_adaptive_packet_header::sptr
ofdm_adaptive_packet_header::make(const std::vector<std::vector<int>>& occupied_carriers,
                                  int n_syms,
                                  const std::string& len_tag_key,
                                  const std::string& frame_len_tag_key,
                                  const std::string& num_tag_key,
                                  int bits_per_header_sym,
                                  bool scramble_header)
{
    return ofdm_adaptive_packet_header::sptr(
        new ofdm_adaptive_packet_header(occupied_carriers,
                                        n_syms,
                                        len_tag_key,
                                        frame_len_tag_key,
                                        num_tag_key,
                                        bits_per_header_sym,
                                        scramble_header));
}


ofdm_adaptive_packet_header::ofdm_adaptive_packet_header(
    const std::vector<std::vector<int>>& occupied_carriers,
    int n_syms,
    const std::string& len_tag_key,
    const std::string& frame_len_tag_key,
    const std::string& num_tag_key,
    int bits_per_header_sym,
    bool scramble_header)
    : packet_header_ofdm(occupied_carriers,
                         n_syms,
                         len_tag_key,
                         frame_len_tag_key,
                         num_tag_key,
                         bits_per_header_sym,
                         0,
                         scramble_header),
      d_constellation_tag_key(get_constellation_tag_key())
{
}

ofdm_adaptive_packet_header::~ofdm_adaptive_packet_header() {}


bool ofdm_adaptive_packet_header::header_formatter(long packet_len,
                                                   unsigned char* out,
                                                   const std::vector<tag_t>& tags)
{
    // Use default
    unsigned int previous_header_number = d_header_number;
    DTL_LOG_TAGS("Before default formatter", tags);
    bool ret_val = packet_header_default::header_formatter(packet_len, out, tags);
    DTL_LOG_TAGS("After default formatter", tags);
    _logger.warn("bits_per_byte: {}, packet_len: {}", d_bits_per_byte, packet_len);
    // Overwrite CRC with constellation type (Bit 24-31 - 8 bits)
    auto it = find_constellation_tag(tags);

    if (it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }

    unsigned char constellation_type_tag_value = pmt::to_long(it->value) & 0xFF;

    int k = 24;
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((constellation_type_tag_value >> i) & d_mask);
    }

    // Re-compute CRC and insert (Bit 32-39 - 8 bits)
    packet_len &= 0x0FFF;
    unsigned char buffer[] = { (unsigned char)(packet_len & 0xFF),
                               (unsigned char)(packet_len >> 8),
                               (unsigned char)(previous_header_number & 0xFF),
                               (unsigned char)(previous_header_number >> 8),
                               (unsigned char)constellation_type_tag_value };

    unsigned char crc = d_crc_impl.compute(buffer, sizeof(buffer));

    _logger.debug("buffer: {0:x} {1:x} {2:x} {3:x} {4:x}",
                  buffer[0],
                  buffer[1],
                  buffer[2],
                  buffer[3],
                  buffer[4]);
    _logger.debug("crc: {0:x}", crc);

    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        out[k] = (unsigned char)((crc >> i) & d_mask);
    }

    // Scramble
    for (int i = 0; i < d_header_len; i++) {
        out[i] ^= d_scramble_mask[i];
    }
    return ret_val;
}

bool ofdm_adaptive_packet_header::header_parser(const unsigned char* in,
                                                std::vector<tag_t>& tags)
{
    DTL_LOG_TAGS("Before default parser", tags);

    packet_header_default::header_parser(in, tags);

    DTL_LOG_TAGS("After default parser", tags);

    // Determine constellation type and add the tag
    int k = 24; // Constellation type starts on bit 24
    unsigned char constellation_type = 0;
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        constellation_type |= (((int)in[k]) & d_mask) << i;
    }
    d_bits_per_payload_sym = compute_no_of_bits_per_symbol(
        static_cast<constellation_type_t>(constellation_type)
    );
    tag_t tag;
    tag.key = get_constellation_tag_key();
    tag.value = pmt::from_long(constellation_type);
    tags.push_back(tag);

    // Compute CRC from tags and verify
    // The packet length in tags is in bytes at this point
    unsigned char crc_calcd = compute_crc(tags);
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        if ((((int)in[k]) & d_mask) != (((int)crc_calcd >> i) & d_mask)) {
            return false;
        }
    }

    // Update length tag to number of symbols in packet
    int no_of_symbols = 0; // # of complex symbols in this frame
    auto it = std::find_if(tags.begin(), tags.end(), [this](auto& t) {
        return pmt::equal(t.key, this->d_len_tag_key);
    });

    if (it == tags.end()) {
        throw std::invalid_argument("Missing packet length tag.");
    }
    no_of_symbols = pmt::to_long(it->value) * 8 / d_bits_per_payload_sym;
    if (pmt::to_long(it->value) * 8 % d_bits_per_payload_sym) {
        no_of_symbols++;
    }
    _logger.debug("d_bits_per_payload_sym: {}, tag_value: {}, no_of_symbols: {}",
                  d_bits_per_payload_sym,
                  pmt::to_long(it->value),
                  no_of_symbols);
    it->value = pmt::from_long(no_of_symbols);

    // Determine frame length and add the tag
    add_frame_length_tag(no_of_symbols, tags);

    DTL_LOG_TAGS("After parser", tags);

    return true;
}


void ofdm_adaptive_packet_header::add_frame_length_tag(int packet_len,
                                                       std::vector<tag_t>& tags)
{
    // To figure out how many payload OFDM symbols there are in this frame,
    // we need to go through the carrier allocation and count the number of
    // allocated carriers per OFDM symbol.
    // frame_len == # of payload OFDM symbols in this frame
    int frame_len = 0;
    size_t k = 0; // position in the carrier allocation map
    int symbols_accounted_for = 0;
    while (symbols_accounted_for < packet_len) {
        frame_len++;
        symbols_accounted_for += d_occupied_carriers[k].size();
        k = (k + 1) % d_occupied_carriers.size();
    }
    tag_t tag;
    tag.key = d_frame_len_tag_key;
    tag.value = pmt::from_long(frame_len);
    tags.push_back(tag);
}


unsigned char ofdm_adaptive_packet_header::compute_crc(std::vector<tag_t>& tags)
{
    int header_len = 0, header_num = 0, constellation_type = -1;
    for (auto& tag : tags) {
        if (pmt::equal(tag.key, d_len_tag_key)) {
            header_len = pmt::to_long(tag.value);
        } else if (pmt::equal(tag.key, d_num_tag_key)) {
            header_num = pmt::to_long(tag.value);
        } else if (pmt::equal(tag.key, d_constellation_tag_key)) {
            constellation_type = pmt::to_long(tag.value);
        }
    }

    unsigned char buffer[] = { (unsigned char)(header_len & 0xFF),
                               (unsigned char)(header_len >> 8),
                               (unsigned char)(header_num & 0xFF),
                               (unsigned char)(header_num >> 8),
                               (unsigned char)constellation_type };
    unsigned char crc_calcd = d_crc_impl.compute(buffer, sizeof(buffer));

    _logger.debug("buffer: {0:x} {1:x} {2:x} {3:x} {4:x}",
                  buffer[0],
                  buffer[1],
                  buffer[2],
                  buffer[3],
                  buffer[4]);
    _logger.debug("crc_calcd: {0:x}", crc_calcd);

    return crc_calcd;
}

} /* namespace dtl */
} /* namespace gr */
