/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_ofdm_adaptive_packet_header_H
#define INCLUDED_DTL_ofdm_adaptive_packet_header_H

#include <gnuradio/digital/packet_header_ofdm.h>
#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/ofdm_adaptive_packet_header.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <vector>


namespace gr {
namespace dtl {


/*!
 * \brief Add constellation type to the OFDM header
 *
 */
class DTL_API ofdm_adaptive_packet_header : public gr::digital::packet_header_ofdm
{
public:
    typedef std::shared_ptr<ofdm_adaptive_packet_header> sptr;

    ofdm_adaptive_packet_header(const std::vector<std::vector<int>>& occupied_carriers,
                                int header_syms,
                                int payload_syms,
                                const std::string& len_tag_key,
                                const std::string& frame_len_tag_key,
                                const std::string& num_tag_key,
                                int bits_per_header_sym,
                                bool scramble_header,
                                bool has_fec);

    virtual ~ofdm_adaptive_packet_header();

    static sptr make(const std::vector<std::vector<int>>& occupied_carriers,
                     int header_syms,
                     int payload_syms,
                     const std::string& len_tag_key,
                     const std::string& frame_len_tag_key,
                     const std::string& num_tag_key,
                     int bits_per_header_sym,
                     bool scramble_header,
                     bool has_fec);

    bool header_formatter(long packet_len,
                          unsigned char* out,
                          const std::vector<tag_t>& tags) override;

    bool header_parser(const unsigned char* in, std::vector<tag_t>& tags) override;

private:
    int
    add_header_field(unsigned char* buf, int offset, unsigned long long val, int n_bits);
    int add_fec_header(const std::vector<tag_t>& tags,
                       unsigned char* out,
                       int first_pos);
    int parse_fec_header(const unsigned char* in,
                         int first_pos,
                         std::vector<tag_t>& tags);

    void pack_crc(const unsigned char* buf_bits, std::vector<unsigned char>& crc_buf);

    pmt::pmt_t d_constellation_tag_key;
    constellation_type_t d_constellation;
    int d_payload_syms;
    bool d_has_fec;
    gr::digital::crc d_crc;
    int d_crc_len;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ofdm_adaptive_packet_header_H */
