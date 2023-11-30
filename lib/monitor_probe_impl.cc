/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "monitor_probe_impl.h"
#include <gnuradio/dtl/monitor_proto.h>
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("monitor_probe");

using namespace std;

message_sender::sptr message_sender::make(char* address, bool bind)
{
    return std::shared_ptr<message_sender>(new message_sender_impl(address, bind));
}


message_sender_impl::message_sender_impl(char* address, bool bind)
    : d_context(1), d_socket(d_context, ZMQ_PUB), counter(0)
{
    int time = 0;
    d_socket.set(zmq::sockopt::linger, time);
    if (bind) {
        d_socket.bind(address);
    } else {
        d_socket.connect(address);
    }
}


void message_sender_impl::send(zmq::message_t* msg)
{
    d_socket.send(*msg,  zmq::send_flags::none);
    ++counter;
}

size_t message_sender_impl::get_msg_counter()
{
    return counter;
}

monitor_probe::sptr
monitor_probe::make(const string& name, message_sender_base::sptr sender)
{
    return gnuradio::make_block_sptr<monitor_probe_impl>(name, sender);
}


monitor_probe_impl::monitor_probe_impl(const string& name, message_sender_base::sptr sender)
    : gr::block(
          "monitor_probe", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_probe_name(name),
      d_sender(sender)
{
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { this->monitor_msg_handler(msg); });
}


monitor_probe_impl::~monitor_probe_impl() {}


size_t monitor_probe_impl::monitor_msg_handler(pmt::pmt_t msg)
{
    size_t sz = 0;
    if (pmt::is_any(msg)) {
        // Handle as proto message
        std::shared_ptr<monitor_proto_msg> proto_msg =
            boost::any_cast<std::shared_ptr<monitor_proto_msg>>(pmt::any_ref(msg));
        proto_msg->set_nmsgs(this->nmsgs(pmt::mp("in")));
        proto_msg->set_sent_counter(d_sender->get_msg_counter());
        sz = 1 + proto_msg->ByteSizeLong();
        zmq::message_t zmq_msg(sz);
        uint8_t* buf = (uint8_t*)zmq_msg.data();
        buf[0] = TAG_PROTO;
        proto_msg->SerializeToArray(buf + 1, proto_msg->ByteSizeLong());
        if (d_sender != nullptr) {
            d_sender->send(&zmq_msg);
        }
    } else if (pmt::is_blob(msg)) {
        pmt::pmt_t carrier_msg =
            pmt::cons(pmt::from_long(d_sender->get_msg_counter()),
                      pmt::cons(pmt::from_long(this->nmsgs(pmt::mp("in"))), msg));
        std::stringbuf buf;
        pmt::serialize(carrier_msg, buf);
        auto s = buf.str();
        sz = s.size();
        zmq::message_t zmq_msg(sz);
        memcpy(zmq_msg.data(), s.c_str(), sz);
        if (d_sender != nullptr) {
            d_sender->send(&zmq_msg);
        }
    } else {
        // Handle as pmt message
        std::stringbuf buf;
        msg = pmt::dict_add(msg,
                            pmt::string_to_symbol("nmsgs"),
                            pmt::from_uint64(this->nmsgs(pmt::mp("in"))));
        msg = pmt::dict_add(msg,
                            pmt::string_to_symbol("sent_counter"),
                            pmt::from_long(d_sender->get_msg_counter()));
        pmt::serialize(msg, buf);
        auto s = buf.str();
        sz = s.size();
        zmq::message_t zmq_msg(sz);
        memcpy(zmq_msg.data(), s.c_str(), sz);
        if (d_sender != nullptr) {
            d_sender->send(&zmq_msg);
        }
    }
    return sz;
}

} /* namespace dtl */
} /* namespace gr */
