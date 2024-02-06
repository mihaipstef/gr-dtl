/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "to_phy_impl.h"
#include <arpa/inet.h>
#include <gnuradio/io_signature.h>
#include <tuple>

#include <iostream>
#include <iomanip>


namespace gr {
namespace dtl {


const pmt::pmt_t pdu_in()
{
    static const pmt::pmt_t val = pmt::mp("pdus");
    return val;
}


to_phy::sptr to_phy::make(transported_protocol_t protocol, const std::string& len_key)
{
    return gnuradio::make_block_sptr<to_phy_impl>(protocol, len_key);
}


/*
 * The private constructor
 */
to_phy_impl::to_phy_impl(transported_protocol_t protocol, const std::string& len_key)
    : gr::tagged_stream_block("to_phy",
                              gr::io_signature::make(0, 0, 0),
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              len_key),
      d_protocol(protocol),
      d_len_key(pmt::mp(len_key)),
      d_pdu_len(0)
{
    message_port_register_in(pdu_in());
}


to_phy_impl::~to_phy_impl() {}


int to_phy_impl::calculate_output_stream_length(const gr_vector_int& ninput_items)
{
     std::cout << "aici" << std::endl;
    if (d_pdu_len == 0) {
        std::cout << "aici2" << std::endl;
        pmt::pmt_t msg(delete_head_nowait(pdu_in()));
        if (msg.get() == NULL) {
            return 0;
        }
        std::cout << "pdu in" << std::endl;
        d_pdu = pmt::cdr(msg);
        d_pdu_len = pmt::blob_length(d_pdu);
        if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
            return 2 + d_pdu_len;
        }
    }
    return d_pdu_len;
}


int to_phy_impl::work(int noutput_items,
                         gr_vector_int& ninput_items,
                         gr_vector_const_void_star& input_items,
                         gr_vector_void_star& output_items)
{
    if (d_pdu_len == 0) {
        return 0;
    }
    auto out = static_cast<uint8_t*>(output_items[0]);
    size_t len = 0;
    const uint8_t* in = (const uint8_t*)uniform_vector_elements(d_pdu, len);
    std::cout << "to_phy=" << noutput_items << std::endl;
    for (int i=0; i<len; ++i) std::cout << " " << std::setfill('0') << std::setw(2) << std::hex << (int)in[i];
    std::cout << std::endl;
    if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
        // Alter ethernet frame
        memcpy(out, in, 12);
        // // add length here
        size_t new_len = len + 2;
        out[12] = (new_len >> 8) & 0xff;
        out[13] = new_len & 0xff;
        memcpy(out+14, in+12, len-12);
        std::cout << "alter mac" << std::endl;
        d_pdu_len = 0;
        return new_len;
    }
    else {
        memcpy(out, in, len);
        d_pdu_len = 0;
        return len;
    }
}


} /* namespace dtl */
} /* namespace gr */
