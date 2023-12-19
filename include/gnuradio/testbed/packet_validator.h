/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_PACKET_VALIDATOR_H
#define INCLUDED_DTL_PACKET_VALIDATOR_H

#include <gnuradio/dtl/api.h>
#include <memory>
#include <string>
#include <vector>

namespace gr {
namespace dtl {


class DTL_API packet_validator
{
public:
    typedef std::shared_ptr<packet_validator> sptr;
    virtual int valid(const uint8_t* buf, size_t len) = 0;
};


class DTL_API ip_validator: public packet_validator
{
    public:
        ip_validator(const std::string& src_addr);
        int valid(const uint8_t* buf, size_t len) override;
    private:
        std::string d_src_addr;
};


class DTL_API ethernet_validator: public packet_validator
{
    public:
        ethernet_validator(const std::string& dst_addr);
        int valid(const uint8_t* buf, size_t len) override;
    private:
        std::vector<u_int8_t> d_dst_addr;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_PACKET_VALIDATOR_H */
