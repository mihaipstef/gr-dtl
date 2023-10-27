/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PARSER_REGISTRY_H
#define INCLUDED_DTL_MONITOR_PARSER_REGISTRY_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/monitor.pb.h>
#include <gnuradio/dtl/monitor_parser.h>
#include <memory>
#include <unordered_map>


namespace gr {
namespace dtl {


typedef std::function<void(monitor_proto_msg*, msg_dict_t*)> parser_t;
typedef void (*parser_ptr_t)(monitor_proto_msg*, msg_dict_t*);
typedef std::unordered_map<msg_type_id_t, parser_t> parser_registry_t;


class DTL_API parser_registry
{

private:
    static parser_registry_t& registry();

    static void register_parser(
        msg_type_id_t msg_id,
        const parser_t& f);

public:

    static void
    call_parser(msg_type_id_t msg_id, monitor_proto_msg* msg, msg_dict_t* result);
};


} // namespace dtl
} // namespace gr

#endif