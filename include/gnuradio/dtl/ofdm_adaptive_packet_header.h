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

    static sptr make(const std::vector<std::vector<int>>& occupied_carriers,
              int n_syms,
              const std::string& len_tag_key,
              const std::string& frame_len_tag_key,
              const std::string& num_tag_key,
              int bits_per_header_sym,
              int bits_per_payload_sym,
              bool scramble_header);

    ofdm_adaptive_packet_header(const std::vector<std::vector<int>>& occupied_carriers,
                                int n_syms,
                                const std::string& len_tag_key,
                                const std::string& frame_len_tag_key,
                                const std::string& num_tag_key,
                                int bits_per_header_sym,
                                int bits_per_payload_sym,
                                bool scramble_header);
    ~ofdm_adaptive_packet_header();

    bool header_formatter(long packet_len,
                          unsigned char* out,
                          const std::vector<tag_t>& tags) override;

    bool header_parser(const unsigned char* in, std::vector<tag_t>& tags) override;

protected:
    void add_frame_length_tag(int packet_len, std::vector<tag_t>& tags);
    unsigned char compute_crc(std::vector<tag_t>& tags);

    pmt::pmt_t d_constellation_tag_key;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ofdm_adaptive_packet_header_H */
