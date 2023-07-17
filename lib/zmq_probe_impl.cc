/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "zmq_probe_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("zmq_probe");

using namespace std;

zmq_probe::sptr zmq_probe::make(char* address,
                                const string& probe_name,
                                const string& collection_name,
                                bool bind)
{
    return gnuradio::make_block_sptr<zmq_probe_impl>(
        address, probe_name, collection_name, bind);
}

zmq_probe_impl::zmq_probe_impl(char* address,
                               const string& probe_name,
                               const string& collection_name,
                               bool bind)
    : gr::block(
          "zmq_probe", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
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

    d_meta_msg = pmt::dict_add(pmt::make_dict(),
                               pmt::string_to_symbol("probe_name"),
                               pmt::string_to_symbol(probe_name));
    d_meta_msg = pmt::dict_add(d_meta_msg,
                               pmt::string_to_symbol("collection"),
                               pmt::string_to_symbol(collection_name));
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { this->handler(msg); });
}

zmq_probe_impl::~zmq_probe_impl() {}

void zmq_probe_impl::handler(pmt::pmt_t msg)
{
    pmt::pmt_t carrier_msg = pmt::dict_add(d_meta_msg, pmt::string_to_symbol("msg"), msg);
    std::string s = pmt::serialize_str(carrier_msg);
    zmq::message_t zmsg(s.size());
    memcpy(zmsg.data(), s.c_str(), s.size());
    d_socket.send(zmsg, zmq::send_flags::none);
}

} /* namespace dtl */
} /* namespace gr */
