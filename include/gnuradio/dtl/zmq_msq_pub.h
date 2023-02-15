/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_ZMQ_MSQ_PUB_H
#define INCLUDED_DTL_ZMQ_MSQ_PUB_H

#include <gnuradio/block.h>
#include <gnuradio/dtl/api.h>

namespace gr {
namespace dtl {

/*!
 * \brief Publish messages
 * \ingroup dtl
 *
 */
class DTL_API zmq_msq_pub : virtual public gr::block
{
public:
    typedef std::shared_ptr<zmq_msq_pub> sptr;
    static sptr make(char* address, bool bind = true);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_ZMQ_MSQ_PUB_H */
