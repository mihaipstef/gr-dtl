find_package(Protobuf REQUIRED)
include(FindProtobuf)

list (APPEND proto_schema_files
    monitor.proto
    monitor_fec.proto)

include_directories(${PROTOBUF_INCLUDE_DIR})
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${proto_schema_files})
add_library(proto SHARED ${PROTO_SRC})
set_target_properties(proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(proto PROPERTIES CXX_VISIBILITY_PRESET default)
GR_LIBRARY_FOO(proto)

