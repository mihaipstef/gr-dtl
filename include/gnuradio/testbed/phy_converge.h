/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_TESTBED_PHY_CONVERGE_H
#define INCLUDED_TESTBED_PHY_CONVERGE_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/tagged_stream_block.h>
#include <gnuradio/testbed/packet_validator.h>

namespace gr {
namespace dtl {


enum class DTL_API transported_protocol_t { IPV4_ONLY = 0, ETHER_IPV4, MODIFIED_ETHER };

class DTL_API from_phy : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<from_phy> sptr;
    static sptr make(transported_protocol_t protocol,
                     packet_validator::sptr validator,
                     const std::string& len_key);
};


class DTL_API to_phy : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<to_phy> sptr;
    static sptr make(transported_protocol_t protocol, const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_TESTBED_PHY_CONVERGE_H */
