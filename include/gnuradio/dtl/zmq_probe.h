/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_ZMQ_PROBE_H
#define INCLUDED_DTL_ZMQ_PROBE_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>

namespace gr {
namespace dtl {

/*!
 * \brief Publish messages
 * \ingroup dtl
 *
 */
class DTL_API zmq_probe : virtual public gr::block
{
public:
    typedef std::shared_ptr<zmq_probe> sptr;
    static sptr make(char* address,
                     const std::string& probe_name,
                     const std::string& collection_name,
                     bool bind = true);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ZMQ_PROBE_H */
