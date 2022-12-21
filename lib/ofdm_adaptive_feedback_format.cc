/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_feedback_format.h>
#include <gnuradio/math.h>
#include <volk/volk_alloc.hh>
#include <cstring>

namespace gr {
namespace dtl {

ofdm_adaptive_feedback_format::sptr
ofdm_adaptive_feedback_format::make(const std::string& access_code, int threshold)
{
    return ofdm_adaptive_feedback_format::sptr(
        new ofdm_adaptive_feedback_format(access_code, threshold));
}

ofdm_adaptive_feedback_format::ofdm_adaptive_feedback_format(
    const std::string& access_code, int threshold)
    : header_format_base(),
      d_data_reg(0),
      d_mask(0),
      d_threshold(threshold),
      d_crc8(8, 0x07, 0xFF, 0x00, false, false)
{
    d_access_code_len = access_code.length(); // # of bits in the access code


    if (d_threshold > d_access_code_len) {
        throw std::runtime_error("ofdm_adaptive_feedback_format: Cannot set threshold "
                                 "larger than the access code length.");
    }

    if (access_code.size() > 64) {
        throw std::runtime_error(
            "ofdm_adaptive_feedback_format: Setting access code failed");
    }

    // set len top bits to 1.
    d_mask = ((~0ULL) >> (64 - d_access_code_len));

    d_access_code = 0;
    for (unsigned i = 0; i < d_access_code_len; i++) {
        d_access_code = (d_access_code << 1) | (access_code[i] & 1);
    }
}

ofdm_adaptive_feedback_format::~ofdm_adaptive_feedback_format() {}


unsigned long long ofdm_adaptive_feedback_format::access_code() const
{
    return d_access_code;
}

unsigned int ofdm_adaptive_feedback_format::threshold() const { return d_threshold; }

bool ofdm_adaptive_feedback_format::format(int nbytes_in,
                                           const unsigned char* input,
                                           pmt::pmt_t& output,
                                           pmt::pmt_t& info)
{
    // Creating the output pmt copies data; free our own here when done.
    volk::vector<uint8_t> bytes_out(header_nbytes());
    gr::digital::header_buffer header(bytes_out.data());
    header.add_field64(d_access_code, d_access_code_len);
    header.add_field8(input[0]);
    header.add_field8(input[1]);
    header.add_field8(d_crc8.compute(input, 2));
    output = pmt::init_u8vector(header_nbytes(), bytes_out.data());
    return true;
}


bool ofdm_adaptive_feedback_format::parse_feedback(int nbits_in,
                                                   const unsigned char* input,
                                                   std::vector<pmt::pmt_t>& info)
{
    int nbits_processed = 0;
    // Insert the bits in the header buffer
    while (nbits_processed <= nbits_in) {
        d_hdr_reg.insert_bit(input[nbits_processed++]);
        if (d_hdr_reg.length() == (header_nbits() - d_access_code_len)) {
            bool header_result = header_ok();
            d_hdr_reg.clear();
            return header_result;
        }
    }
    return false;
}


bool ofdm_adaptive_feedback_format::parse(int nbits_in,
                                          const unsigned char* input,
                                          std::vector<pmt::pmt_t>& info,
                                          int& nbits_processed)
{
    nbits_processed = 0;
    // Look for the access code in input bits and parse the feedback
    while (nbits_processed < nbits_in) {
        d_data_reg = (d_data_reg << 1) | ((input[nbits_processed++]) & 0x1);
        // Compute hamming distance between desired access code and current data
        uint64_t wrong_bits = 0;
        uint64_t nwrong = d_threshold + 1;
        wrong_bits = (d_data_reg ^ d_access_code) & d_mask;
        volk_64u_popcnt(&nwrong, wrong_bits);
        // If access code found...
        if (nwrong <= d_threshold) {
            //... parse feedback.
            return parse_feedback(
                nbits_in - nbits_processed, &input[nbits_processed], info);
        }
    }
    return false;
}

size_t ofdm_adaptive_feedback_format::header_nbits() const
{
    return d_access_code_len + 8 * 3;
}

bool ofdm_adaptive_feedback_format::header_ok()
{
    unsigned char constellation_type = d_hdr_reg.extract_field16(0, 8);
    unsigned char fec_scheme = d_hdr_reg.extract_field16(8, 16);
    unsigned char crc = d_hdr_reg.extract_field16(16, 24);
    unsigned char buffer[] = { constellation_type, fec_scheme };
    uint8_t crc_clcd = d_crc8.compute(buffer, sizeof(buffer));
    return crc_clcd == crc;
}

int ofdm_adaptive_feedback_format::header_payload()
{
    //There is no payload.
    return 0;
}

} // namespace dtl
} // namespace gr