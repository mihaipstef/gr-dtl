/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <gnuradio/testbed/packet_validator.h>
#include <netinet/ip.h>
#include <cstring>
#include <string>


namespace gr {
namespace dtl {

ip_validator::ip_validator(const std::string& src_addr) : d_src_addr(src_addr) {}


int ip_validator::valid(const uint8_t* buf, size_t len)
{
    struct ip* iph = (struct ip*)buf;
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

    // Set the header checksum value from cache and validate
    iph->ip_sum = header_sum;
    if (htons(~sum) == header_sum && header_sum != 0) {
        return ntohs(iph->ip_len);
    }
    return -1;
}

ethernet_validator::ethernet_validator(const std::string& dst_addr) : d_dst_addr(6, 0)
{
    sscanf(dst_addr.c_str(),
           "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &d_dst_addr[0],
           &d_dst_addr[1],
           &d_dst_addr[2],
           &d_dst_addr[3],
           &d_dst_addr[4],
           &d_dst_addr[5]);
}


int ethernet_validator::valid(const uint8_t* buf, size_t len)
{
    if (len < 14) {
        return -1;
    }

    bool ether_start = (0 == memcmp(buf, &d_dst_addr[0], 6));
    if (ether_start) {
        return 14 + ntohs(*((uint16_t*)(buf + 16)));
    }
    return -1;
}


} /* namespace dtl */
} /* namespace gr */