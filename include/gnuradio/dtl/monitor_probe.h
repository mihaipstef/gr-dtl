/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROBE_H
#define INCLUDED_DTL_MONITOR_PROBE_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>
#include <zmq.hpp>

namespace gr {
namespace dtl {


class DTL_API message_sender_base {
public:
    typedef std::shared_ptr<message_sender_base> sptr;
    virtual void send(zmq::message_t* msg) = 0;
    virtual size_t get_msg_counter() = 0;
};


class DTL_API message_sender: virtual public message_sender_base {
public:
    typedef std::shared_ptr<message_sender> sptr;
    static message_sender::sptr make(char* address, bool bind);
};


/*!
 * \brief Publish messages
 * \ingroup dtl
 *
 */
class DTL_API monitor_probe : virtual public gr::block
{
public:
    typedef std::shared_ptr<monitor_probe> sptr;
    static sptr make(const std::string& name, message_sender_base::sptr sender);
    virtual size_t monitor_msg_handler(pmt::pmt_t msg) = 0;
};



} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_MONITOR_PROBE_H */
