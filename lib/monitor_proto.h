/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROTO_H
#define INCLUDED_DTL_MONITOR_PROTO_H


#include <gnuradio/dtl/api.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>
#include <pmt/pmt.h>
#include "proto/monitor.pb.h"


namespace gr {
namespace dtl {


using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;


uint64_t system_ts();


#define HANDLE_TYPE(PAYLOAD, CPPTYPE, TYPE, METHOD, FD, V)        \
    case FieldDescriptor::CPPTYPE_##CPPTYPE: {                    \
        TYPE tmp = (TYPE)V;                                       \
        payload_reflection->Set##METHOD((PAYLOAD), (FD), (tmp));  \
        break;                                                    \
    }


template <class M>
class monitor_proto
{

private:
    M payload;
    monitor_proto_msg ts_msg;
    const Reflection* payload_reflection;
    std::unordered_map<std::string, const FieldDescriptor*> fields_by_name;
    pmt::pmt_t pmt_blob;
    uint8_t* blob_internal_buf;
    size_t blob_internal_len;

    template <class T>
    void set_payload_field(const T& p) {
        auto it = fields_by_name.find(p.first);
        if (it != fields_by_name.end()) {
            switch (it->second->cpp_type()) {

                default:
                    break;

                HANDLE_TYPE(&payload, INT32 , int32_t,     Int32 , it->second, p.second);
                HANDLE_TYPE(&payload, INT64 , int64_t,     Int64 , it->second, p.second);
                HANDLE_TYPE(&payload, UINT32, uint32_t,    UInt32, it->second, p.second);
                HANDLE_TYPE(&payload, UINT64, uint32_t,    UInt64, it->second, p.second);
                // String support needs some sort of type erasure on integral and floating point branches to compile
                //HANDLE_TYPE(STRING, std::string, String, it->second, p.second);
                HANDLE_TYPE(&payload, DOUBLE, double,      Double, it->second, p.second);
                HANDLE_TYPE(&payload, FLOAT , float,       Float , it->second, p.second);
                HANDLE_TYPE(&payload, BOOL  , bool,        Bool  , it->second, p.second);
            }

        }
    }

public:

    monitor_proto() {
        payload_reflection = payload.GetReflection();
        const Descriptor * descriptor = payload.GetDescriptor();
        for(int i = 0; i < descriptor->field_count(); i++) {
            const FieldDescriptor * fd = descriptor->field(i);
            fields_by_name.insert(std::make_pair(fd->name(), fd));
        }
        size_t sz = ts_msg.SpaceUsedLong() + payload.SpaceUsedLong();
        std::vector<uint8_t> tmp_buf(sz);
        pmt_blob = pmt::make_blob(&tmp_buf[0], sz);
        blob_internal_len = blob_length(pmt_blob);
        blob_internal_buf = (uint8_t*)const_cast<void*>(blob_data(pmt_blob));
    }


    template <class... P>
    pmt::pmt_t build(P ...pairs) {
        (set_payload_field(pairs), ...);
        ts_msg.set_ts(system_ts());
        ts_msg.mutable_payload()->PackFrom(payload);
        ts_msg.SerializeToArray(blob_internal_buf, ts_msg.ByteSizeLong());
        return pmt_blob;
    }


    std::pair<uint64_t, M*> parse(pmt::pmt_t& pmt_blob) {
        if (pmt::is_blob(pmt_blob)) {
            ts_msg.ParseFromArray(blob_data(pmt_blob), blob_length(pmt_blob));
            if (ts_msg.has_payload()) {
                 ts_msg.payload().UnpackTo(&payload);
                return std::make_pair(ts_msg.ts(), &payload);
            }
            return std::make_pair(ts_msg.ts(), nullptr);
        }
        return std::make_pair(0, nullptr);
    }

};


} // namespace dtl
} // namespace gr

#endif