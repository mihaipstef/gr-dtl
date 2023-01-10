/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dtl/ofdm_adaptive_feedback_format.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/math.h>
#include <cstring>
#include "logger.h"
#include <volk/volk_alloc.hh>


namespace gr {
namespace dtl {

using namespace gr::digital;

INIT_DTL_LOGGER("ofdm_adaptive_feedback_format");

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
      d_crc8(8, 0x07, 0xFF, 0x00, false, false),
      d_feedback_ok(false)
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
    header_buffer header(bytes_out.data());
    header.add_field64(d_access_code, d_access_code_len);
    header.add_field8(input[0]);
    header.add_field8(input[1]);
    header.add_field8(d_crc8.compute(input, 2));
    output = pmt::init_u8vector(header_nbytes(), bytes_out.data());
    DTL_LOG_DEBUG("Format feedback ok", 0);
    return true;
}


bool ofdm_adaptive_feedback_format::parse_feedback(int nbits_in,
                                                   const unsigned char* input,
                                                   std::vector<pmt::pmt_t>& info,
                                                   int& nbits_processed)
{
    d_hdr_reg.clear();

    // Insert the bits in the header buffer
    while (nbits_processed < nbits_in) {
        d_hdr_reg.insert_bit(input[nbits_processed++]);
        if (d_hdr_reg.length() == (header_nbits() - d_access_code_len)) {
            unsigned char constellation_type = d_hdr_reg.extract_field8(0, 8);
            unsigned char fec_scheme = d_hdr_reg.extract_field8(8, 8);
            unsigned char crc = d_hdr_reg.extract_field8(16, 8);
            unsigned char buffer[] = { constellation_type, fec_scheme };
            uint8_t crc_clcd = d_crc8.compute(buffer, sizeof(buffer));
            if (crc_clcd == crc) {
                d_feedback_ok = true;
                pmt::pmt_t parsed_feedback = pmt::make_dict();
                parsed_feedback = pmt::dict_add(parsed_feedback, feedback_constellation_key(), pmt::from_long(constellation_type));
                parsed_feedback = pmt::dict_add(parsed_feedback, feedback_fec_key(), pmt::from_long(fec_scheme));
                info.push_back(parsed_feedback);
            }
            DTL_LOG_DEBUG("Parsed feedback: {} ({})", d_feedback_ok, nbits_in);
            return d_feedback_ok;
        }
    }
    DTL_LOG_DEBUG("Parsed feedback: not enough ({})", nbits_in);
    return false;
}


bool ofdm_adaptive_feedback_format::parse(int nbits_in,
                                          const unsigned char* input,
                                          std::vector<pmt::pmt_t>& info,
                                          int& nbits_processed)
{
    nbits_processed = 0;
    d_feedback_ok = false;
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
            //... and parsed feedback ok return success. Otherwise, continue.
            DTL_LOG_DEBUG("Detect feedback: {}/{}", nbits_in - nbits_processed, nbits_in);
            if (parse_feedback(
                    nbits_in, input, info, nbits_processed)) {
                return true;
            }
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
    return d_feedback_ok;
}

int ofdm_adaptive_feedback_format::header_payload()
{
    // There is no payload.
    return 0;
}

} // namespace dtl
} // namespace gr