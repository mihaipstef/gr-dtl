/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_packet_header.h>

#include <gnuradio/io_signature.h>
#include <gnuradio/logger.h>

#include <algorithm>

namespace gr {
namespace dtl {

using namespace gr::digital;


gr::logger _logger(__FILE__);


ofdm_adaptive_packet_header::sptr
ofdm_adaptive_packet_header::make(const std::vector<std::vector<int>>& occupied_carriers,
                                  int n_syms,
                                  const std::string& len_tag_key,
                                  const std::string& frame_len_tag_key,
                                  const std::string& num_tag_key,
                                  int bits_per_header_sym,
                                  int bits_per_payload_sym,
                                  bool scramble_header)
{
    return ofdm_adaptive_packet_header::sptr(
        new ofdm_adaptive_packet_header(occupied_carriers,
                                        n_syms,
                                        len_tag_key,
                                        frame_len_tag_key,
                                        num_tag_key,
                                        bits_per_header_sym,
                                        bits_per_payload_sym,
                                        scramble_header));
}


ofdm_adaptive_packet_header::ofdm_adaptive_packet_header(
    const std::vector<std::vector<int>>& occupied_carriers,
    int n_syms,
    const std::string& len_tag_key,
    const std::string& frame_len_tag_key,
    const std::string& num_tag_key,
    int bits_per_header_sym,
    int bits_per_payload_sym,
    bool scramble_header)
    : packet_header_ofdm(occupied_carriers,
                         n_syms,
                         len_tag_key,
                         frame_len_tag_key,
                         num_tag_key,
                         bits_per_header_sym,
                         bits_per_payload_sym,
                         scramble_header),
      d_constellation_tag_key(pmt::string_to_symbol("frame_constellation"))
{
}

ofdm_adaptive_packet_header::~ofdm_adaptive_packet_header() {}


bool ofdm_adaptive_packet_header::header_formatter(long packet_len,
                                                   unsigned char* out,
                                                   const std::vector<tag_t>& tags)
{
    unsigned int previous_header_number = d_header_number;
    bool ret_val = packet_header_default::header_formatter(packet_len, out, tags);
    _logger.warn("bits_per_byte: {}", d_bits_per_byte);
    // Overwrite CRC with constellation type (Bit 24-31 - 8 bits)
    auto it = std::find_if(tags.begin(), tags.end(), [](auto& t) {
        return t.key == pmt::string_to_symbol("frame_constellation");
    });

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
    packet_header_default::header_parser(in, tags);

    // Convert bytes to complex symbols:
    int packet_len = 0; // # of complex symbols in this frame
    for (size_t i = 0; i < tags.size(); i++) {
        if (pmt::equal(tags[i].key, d_len_tag_key)) {
            packet_len = pmt::to_long(tags[i].value) * 8 / d_bits_per_payload_sym;
            if (pmt::to_long(tags[i].value) * 8 % d_bits_per_payload_sym) {
                packet_len++;
            }
            tags[i].value = pmt::from_long(packet_len);
            break;
        }
    }

    // Determine frame length and add the tag
    add_frame_length_tag(packet_len, tags);

    // Determine constellation type and add the tag
    int k = 24; // Constellation type starts on bit 24
    unsigned char constellation_type = 0;
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        constellation_type |= (((int)in[k]) & d_mask) << i;
    }

    tag_t tag;
    tag.key = pmt::string_to_symbol("frame_constellation");
    tag.value = pmt::from_long(constellation_type);
    tags.push_back(tag);

    // Compute CRC from tags and verify
    unsigned char crc_calcd = compute_crc(tags);
    for (int i = 0; i < 8 && k < d_header_len; i += d_bits_per_byte, k++) {
        if ((((int)in[k]) & d_mask) != (((int)crc_calcd >> i) & d_mask)) {
            return false;
        }
    }

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
