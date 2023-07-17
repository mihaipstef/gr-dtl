/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_ZMQ_PROBE_IMPL_H
#define INCLUDED_DTL_ZMQ_PROBE_IMPL_H

#include <gnuradio/dtl/zmq_probe.h>
#include <zmq.hpp>

namespace gr {
namespace dtl {

class zmq_probe_impl : public zmq_probe
{
private:
    zmq::context_t d_context;
    zmq::socket_t d_socket;
    pmt::pmt_t d_meta_msg;

public:
    zmq_probe_impl(char* address,
                   const std::string& probe_name,
                   const std::string& collection_name,
                   bool bind);
    ~zmq_probe_impl() override;

    void handler(pmt::pmt_t msg);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ZMQ_PROBE_IMPL_H */
