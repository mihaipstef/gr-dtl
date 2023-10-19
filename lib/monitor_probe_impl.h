/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROBE_IMPL_H
#define INCLUDED_DTL_MONITOR_PROBE_IMPL_H

#include <gnuradio/dtl/monitor_probe.h>

namespace gr {
namespace dtl {


class message_sender_impl : public message_sender
{
private:
    zmq::context_t d_context;
    zmq::socket_t d_socket;
public:
    message_sender_impl(char* address,
                   bool bind);
    void send(zmq::message_t* msg) override;
};


class monitor_probe_impl : public monitor_probe
{
private:
    std::string d_probe_name;
    message_sender_base::sptr d_sender;
    pmt::pmt_t d_meta_msg;
public:
    monitor_probe_impl(const std::string& name, message_sender_base::sptr sender);
    ~monitor_probe_impl() override;

    void monitor_msg_handler(pmt::pmt_t msg) override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_MONITOR_PROBE_IMPL_H */
