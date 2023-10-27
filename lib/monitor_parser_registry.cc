#include <gnuradio/dtl/monitor_parser.h>
#include <gnuradio/dtl/monitor_parser_registry.h>
#include "monitor_proto_messages.h"


namespace gr {
namespace dtl {


parser_registry_t&  parser_registry::registry() {
    static parser_registry_t _registry = {
        { proto_messages::FEC_DEC_MSG, &proto_fec_t::parser::parse }
    };
    return _registry;
}


void parser_registry::call_parser(msg_type_id_t msg_id, monitor_proto_msg* msg, msg_dict_t* result) {
    auto parser_it = registry().find(msg_id);
    if (parser_it != registry().end()) {
        parser_it->second(msg, result);
    } else {
        std::cout << "No parser registered for msg_id=" << msg_id << std::endl; 
    }
}


void parser_registry::register_parser(msg_type_id_t msg_id,
                                      const parser_t& f)
{
    auto it = registry().find(msg_id);
    if (it == registry().end()) {
        registry()[msg_id] = f;
    } else {
        if (f.target<void(monitor_proto_msg*, std::map<std::string, std::string>&)>() !=
            it->second.target<void(monitor_proto_msg*,
                                   std::map<std::string, std::string>&)>()) {
            throw std::runtime_error("A parser was already registered");
            //std::cout << "A parser was already registered" << std::endl;
        }
    }
}


} // namespace dtl
} // namespace gr