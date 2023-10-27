#include "proto/monitor_fec.pb.h"
#include <gnuradio/dtl/monitor_parser.h>
#include <gnuradio/dtl/monitor_parser_registry.h>


namespace gr {
namespace dtl {


msg_encoding_t parse(uint8_t* data, size_t size, parse_result& result)
{
    if (size) {
        if (data[0] == TAG_PROTO) {
            std::shared_ptr<monitor_proto_msg> parsed_msg =
                std::make_shared<monitor_proto_msg>();
            parsed_msg->ParseFromArray(&data[1], size - 1);
            populate(parsed_msg.get(), &(result.dict_msg));
            // execute registered payload parser
            parser_registry::call_parser(parsed_msg->proto_id(), parsed_msg.get(), &(result.dict_msg));
            return msg_encoding_t::PROTO;
        } else {
            std::string s((char*)data, size);
            std::stringbuf sb(s);
            pmt::pmt_t pmt_msg = pmt::deserialize(sb);
            if (pmt::is_pair(pmt_msg)) {
                pmt::pmt_t blob = pmt::cdr(pmt::cdr(pmt_msg));
                if (pmt::is_blob(blob)) {
                    std::shared_ptr<monitor_proto_msg> parsed_msg =
                        std::make_shared<monitor_proto_msg>();
                    parsed_msg->ParseFromArray(blob_data(blob), blob_length(blob));
                    populate(parsed_msg.get(), &(result.dict_msg));
                    // execute registered payload parser
                    parser_registry::call_parser(parsed_msg->proto_id(), parsed_msg.get(),
                                                                        &(result.dict_msg));
                    return msg_encoding_t::PROTO_IN_BLOB;
                } else {
                    result.pmt_msg = pmt_msg;
                    return msg_encoding_t::PMT;
                }
            }
        }
    }
    return msg_encoding_t::UNKNOWN;
}


void populate(google::protobuf::Message* msg, msg_dict_t* result)
{
    const google::protobuf::Reflection* payload_reflection = msg->GetReflection();
    const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
    for (int i = 0; i < descriptor->field_count(); i++) {
        const google::protobuf::FieldDescriptor* fd = descriptor->field(i);
        switch (fd->type()) {
        default:
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetString(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetInt32(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetInt64(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetUInt32(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            result->insert(std::make_pair(
                fd->name(), std::to_string(payload_reflection->GetUInt64(*msg, fd))));
            break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetFloat(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetDouble(*msg, fd)));
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            result->insert(
                std::make_pair(fd->name(), payload_reflection->GetBool(*msg, fd)));
            break;
        }
    }
}


} // namespace dtl
} // namespace gr