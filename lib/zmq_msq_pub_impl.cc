/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include "zmq_msq_pub_impl.h"

namespace gr {
namespace dtl {

zmq_msq_pub::sptr zmq_msq_pub::make(char* address, bool bind)
{
    return gnuradio::make_block_sptr<zmq_msq_pub_impl>(address, bind);
}

zmq_msq_pub_impl::zmq_msq_pub_impl(char* address, bool bind)
    : gr::block("zmq_msq_pub",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
    d_context(1),
    d_socket(d_context, ZMQ_PUB)
{
    int time = 0;
    d_socket.set(zmq::sockopt::linger, time);
    if (bind) {
        d_socket.bind(address);
    } else {
        d_socket.connect(address);
    }
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { this->handler(msg); });
}

zmq_msq_pub_impl::~zmq_msq_pub_impl() {}

void zmq_msq_pub_impl::handler(pmt::pmt_t msg)
{
    std::string s = pmt::serialize_str(msg);
    zmq::message_t zmsg(s.size());
    memcpy(zmsg.data(), s.c_str(), s.size());
    d_socket.send(zmsg, zmq::send_flags::none);
}

} /* namespace dtl */
} /* namespace gr */
