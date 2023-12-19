/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROBE_PARSER_H
#define INCLUDED_DTL_MONITOR_PROBE_PARSER_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/testbed/monitor.pb.h>
#include <gnuradio/testbed/monitor_probe.h>
#include <map>
#include <memory>
#include <pmt/pmt.h>
#include <string>
#include <variant>

namespace gr {
namespace dtl {


#define TAG_PROTO 0x5c

class non_copyable {
public:
    non_copyable() = default;
    ~non_copyable() = default;
    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;
};


typedef std::unordered_map<
    std::string,
    std::variant<long, double, bool, std::string, non_copyable>>
    msg_dict_t;
typedef std::variant<std::shared_ptr<msg_dict_t>, pmt::pmt_t> msg_t;
typedef uint16_t msg_type_id_t;


enum class DTL_API msg_encoding_t { UNKNOWN = 0, PROTO, PROTO_IN_BLOB, PMT };


void DTL_API populate(google::protobuf::Message* msg, msg_dict_t* result);


class DTL_API parse_result {

public:

    pmt::pmt_t pmt_msg;
    msg_dict_t dict_msg;
    msg_encoding_t encoding;

    msg_dict_t& dict()
    {
        assert(encoding != msg_encoding_t::PMT);
        return dict_msg;
    }

    ::pmt::pmt_t& pmt()
    {
        assert(encoding != msg_encoding_t::PMT);
        return pmt_msg;
    }

};


msg_encoding_t DTL_API parse(uint8_t* data, size_t size, parse_result& result);


template <class M, msg_type_id_t msg_id>
class payload_parser
{
public:
    static void parse(monitor_proto_msg* msg, msg_dict_t* result)
    {
        std::shared_ptr<M> parsed_payload = std::make_shared<M>();
        msg->payload().UnpackTo(parsed_payload.get());
        populate(parsed_payload.get(), result);
    }
};


} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_MONITOR_PROBE_PARSER_H */
