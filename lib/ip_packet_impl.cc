/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ip_packet_impl.h"
#include "logger.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ip_packet");

ip_packet::sptr ip_packet::make(const std::string& len_key)
{
    return gnuradio::make_block_sptr<ip_packet_impl>(len_key);
}

/*
 * The private constructor
 */
ip_packet_impl::ip_packet_impl(const std::string& len_key)
    : gr::tagged_stream_block("ip_packet",
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              len_key),
      d_current_packet_len(0),
      d_expected_len(0),
      d_len_key(pmt::mp(len_key))
{
}


ip_packet_impl::~ip_packet_impl() {}


bool ip_packet_impl::valid_ip_header(struct ip* iph)
{
    // Cache header checksum and reset for calulation
    uint16_t header_sum = iph->ip_sum;
    iph->ip_sum = 0;
    uint32_t sum = 0xffff;
    // Handle complete 16-bit blocks
    size_t header_len = iph->ip_hl * 2;
    uint16_t* data = (uint16_t*)iph;
    for (size_t i = 0; i < header_len; ++i) {
        sum += ntohs((uint16_t)data[i]);
        if (sum > 0xffff) {
            sum -= 0xffff;
        }
    }
    DTL_LOG_DEBUG("valid_ip_header: calc_sum={}, header_sum={}, id={}, len={}",
                  htons(~sum),
                  header_sum,
                  ntohs(iph->ip_id),
                  ntohs(iph->ip_len));
    // Set the header checksum value from cache and validate
    iph->ip_sum = header_sum;
    return htons(~sum) == header_sum && header_sum != 0;
}


int ip_packet_impl::work(int noutput_items,
                         gr_vector_int& ninput_items,
                         gr_vector_const_void_star& input_items,
                         gr_vector_void_star& output_items)
{
    auto in = static_cast<const uint8_t*>(input_items[0]);
    auto out = static_cast<uint8_t*>(output_items[0]);

    struct ip* iph = (struct ip*)&in[0];
    auto valid_header = valid_ip_header(iph);

    DTL_LOG_DEBUG("ninput={}, noutput={}, d_packet_len={}, expected_len={}",
                  ninput_items[0],
                  noutput_items,
                  d_current_packet_len,
                  d_expected_len);

    if (valid_header) {
        if (d_current_packet_len) {
            memcpy(&out[d_current_packet_len], in, ninput_items[0]);
            size_t produced = d_current_packet_len;
            d_current_packet_len = ninput_items[0];
            DTL_LOG_DEBUG("produced={}", produced);

            return produced;
        }
        d_expected_len = ntohs(iph->ip_len);
    }

    memcpy(&out[d_current_packet_len], in, ninput_items[0]);
    d_current_packet_len += ninput_items[0];
    if (d_current_packet_len < d_expected_len) {
        return 0;
    }

    size_t produced = d_expected_len;

    DTL_LOG_DEBUG("produced={}", produced);

    d_current_packet_len = 0;
    d_expected_len = 0;
    return produced;
}

} /* namespace dtl */
} /* namespace gr */
