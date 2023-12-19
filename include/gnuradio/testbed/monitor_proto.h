/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROTO_H
#define INCLUDED_DTL_MONITOR_PROTO_H


#include <gnuradio/testbed/monitor.pb.h>
#include <gnuradio/testbed/monitor_registry.h>
#include <gnuradio/testbed/monitor_parser.h>
#include <gnuradio/dtl/api.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>
#include <pmt/pmt.h>


namespace gr {
namespace dtl {

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;


uint64_t system_ts();

#define HANDLE_TYPE(PAYLOAD, CPPTYPE, TYPE, METHOD, FD, V)       \
    case FieldDescriptor::CPPTYPE_##CPPTYPE: {                   \
        TYPE tmp = (TYPE)V;                                      \
        payload_reflection->Set##METHOD((PAYLOAD), (FD), (tmp)); \
        break;                                                   \
    }


template <class M, msg_type_id_t msg_id>
class monitor_proto
{

private:
    std::shared_ptr<M> payload;
    std::shared_ptr<monitor_proto_msg> ts_msg;
    const Reflection* payload_reflection;
    std::unordered_map<std::string, const FieldDescriptor*> fields_by_name;
    std::vector<uint8_t> blob_internal_buf;

    template <class T>
    void set_payload_field(const T& p)
    {
        auto it = fields_by_name.find(p.first);
        if (it != fields_by_name.end()) {
            switch (it->second->cpp_type()) {

            default:
                break;

                HANDLE_TYPE(payload.get(), INT32, int32_t, Int32, it->second, p.second);
                HANDLE_TYPE(payload.get(), INT64, int64_t, Int64, it->second, p.second);
                HANDLE_TYPE(payload.get(), UINT32, uint32_t, UInt32, it->second, p.second);
                HANDLE_TYPE(payload.get(), UINT64, uint32_t, UInt64, it->second, p.second);
                // String support needs some sort of type erasure on integral and floating
                // point branches to compile
                // HANDLE_TYPE(STRING, std::string, String, it->second, p.second);
                HANDLE_TYPE(payload.get(), DOUBLE, double, Double, it->second, p.second);
                HANDLE_TYPE(payload.get(), FLOAT, float, Float, it->second, p.second);
                HANDLE_TYPE(payload.get(), BOOL, bool, Bool, it->second, p.second);
            }
        }
    }


public:

    typedef std::shared_ptr<monitor_proto<M, msg_id>> sptr;
    typedef payload_parser<M, msg_id> parser;

    static const msg_type_id_t proto_msg_id = msg_id;

    monitor_proto()
    {
        payload = std::make_shared<M>();
        payload_reflection = payload->GetReflection();
        const Descriptor* descriptor = payload->GetDescriptor();
        for (int i = 0; i < descriptor->field_count(); i++) {
            const FieldDescriptor* fd = descriptor->field(i);
            fields_by_name.insert(std::make_pair(fd->name(), fd));
        }
        ts_msg = std::make_shared<monitor_proto_msg>();
        size_t sz = ts_msg->SpaceUsedLong() + payload->SpaceUsedLong();
        blob_internal_buf.resize(sz);
        //parser_registry::register_parser(msg_id, &payload_parser<M, msg_id>::parse);
    }

    size_t size() {
        return ts_msg->SpaceUsedLong() + payload->SpaceUsedLong();
    }

    template <class... P>
    pmt::pmt_t build(P... pairs)
    {
        // We reuse the message because is already serialized in blob
        ts_msg = std::make_shared<monitor_proto_msg>();
        payload = std::make_shared<M>();
        (set_payload_field(pairs), ...);
        ts_msg->set_time(system_ts());
        ts_msg->set_proto_id(msg_id);
        ts_msg->mutable_payload()->PackFrom(*payload);
        ts_msg->SerializeToArray(&blob_internal_buf[0], ts_msg->ByteSizeLong());
        return pmt::make_blob(&blob_internal_buf[0], ts_msg->ByteSizeLong());
    }


    template <class... P>
    pmt::pmt_t build_any(P... pairs)
    {
        ts_msg = std::make_shared<monitor_proto_msg>();
        payload = std::make_shared<M>();
        (set_payload_field(pairs), ...);
        ts_msg->set_time(system_ts());
        ts_msg->set_proto_id(msg_id);
        ts_msg->mutable_payload()->PackFrom(*payload);
        boost::any any_msg = ts_msg;
        return pmt::make_any(any_msg);
    }

};



} // namespace dtl
} // namespace gr

#endif