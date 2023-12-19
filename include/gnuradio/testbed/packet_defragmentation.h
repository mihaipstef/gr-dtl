/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_IP_PACKET_H
#define INCLUDED_DTL_IP_PACKET_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/tagged_stream_block.h>
#include <gnuradio/testbed/packet_validator.h>

namespace gr {
namespace dtl {

/*!
 * \brief <+description of block+>
 * \ingroup dtl
 *
 */
class DTL_API packet_defragmentation : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<packet_defragmentation> sptr;
    static sptr make(packet_validator::sptr validator, const std::string& len_key);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_IP_PACKET_H */
