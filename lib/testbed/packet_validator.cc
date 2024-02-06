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

#include <iomanip>
#include <iostream>


namespace gr {
namespace dtl {


ip_validator::ip_validator(const std::string& src_addr) : d_src_addr(src_addr) {}


packet_validator::validation_result ip_validator::valid(const uint8_t* buf, size_t len)
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
    bool ip_valid = htons(~sum) == header_sum && header_sum != 0;
    return std::make_tuple(ip_valid, ntohs(iph->ip_len));
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


packet_validator::validation_result ethernet_validator::valid(const uint8_t* buf,
                                                              size_t len)
{
    if (len < 14) {
        return std::make_tuple(false, len);
    }

    std::cout
        << "packet_in="; //<< std::hex << ntohs(*((uint16_t*)(buf + 12))) << std::endl;
    for (int i = 0; i < len; ++i)
        std::cout << " " << std::setfill('0') << std::setw(2) << std::hex << (int)buf[i];
    std::cout << std::endl;

    bool ether_start = (0 == memcmp(buf, &d_dst_addr[0], 6));
    size_t packet_len = 14 + ntohs(*((uint16_t*)(buf + 16)));
    return std::make_tuple(ether_start, packet_len);
}


modified_ethernet_validator::modified_ethernet_validator(const std::string& dst_addr)
    : d_dst_addr(6, 0)
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


packet_validator::validation_result modified_ethernet_validator::valid(const uint8_t* buf,
                                                                       size_t len)
{
    if (len < 14) {
        return std::make_tuple(false, len);
    }

    bool ether_start = (0 == memcmp(buf, &d_dst_addr[0], 6));
    size_t packet_len = ntohs(*((uint16_t*)(buf + 12)));
    std::cout << "[from_phy] packet_in="; //<< std::hex << ntohs(*((uint16_t*)(buf + 12))) << std::endl;
    for (int i = 0; i < len; ++i)
        std::cout << " " << std::setfill('0') << std::setw(2) << std::hex << (int)buf[i];
    std::cout << std::endl;
    return std::make_tuple(ether_start, packet_len);
}


} /* namespace dtl */
} /* namespace gr */