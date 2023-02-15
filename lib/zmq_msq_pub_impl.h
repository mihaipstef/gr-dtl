/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_ZMQ_MSQ_PUB_IMPL_H
#define INCLUDED_DTL_ZMQ_MSQ_PUB_IMPL_H

#include <gnuradio/dtl/zmq_msq_pub.h>
#include <zmq.hpp>

namespace gr {
namespace dtl {

class zmq_msq_pub_impl : public zmq_msq_pub
{
private:
    zmq::context_t d_context;
    zmq::socket_t d_socket;
public:
    zmq_msq_pub_impl(char* address, bool bind);
    ~zmq_msq_pub_impl();

    void handler(pmt::pmt_t msg);

};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ZMQ_MSQ_PUB_IMPL_H */
