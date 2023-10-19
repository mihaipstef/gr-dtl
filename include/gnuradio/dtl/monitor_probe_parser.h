/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROBE_PARSER_H
#define INCLUDED_DTL_MONITOR_PROBE_PARSER_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/monitor_probe.h>
#include <gnuradio/dtl/monitor_proto.h>
#include <memory>
#include <gnuradio/dtl/monitor.pb.h>


namespace gr {
namespace dtl {


template<class M>
std::pair<std::shared_ptr<monitor_proto_msg>, std::shared_ptr<M>>
parse_monitor_msg(uint8_t* data, size_t size)
{
    if (size) {
        if (data[0] == TAG_PROTO) {
            monitor_proto<M> msg;
            return msg.parse_proto(&data[1], size - 1);
        } else {
            monitor_proto<M> msg;
            std::string s((char*)data, size);
            std::stringbuf sb(s);
            pmt::pmt_t pmt_msg = pmt::deserialize(sb);
            if (pmt::is_pair(pmt_msg)) {
                pmt::pmt_t blob = pmt::cdr(pmt::cdr(pmt_msg));
                return msg.parse_pmt(blob);
            }
        }
    }
    return std::make_pair(nullptr, nullptr);
}

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_MONITOR_PROBE_PARSER_H */
