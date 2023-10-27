find_package(Protobuf REQUIRED)
include(FindProtobuf)

list (APPEND proto_schema_files
    monitor.proto
    monitor_fec.proto)

include_directories(${PROTOBUF_INCLUDE_DIR})
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${proto_schema_files})
add_library(dtl-proto SHARED ${PROTO_SRC})
set_target_properties(dtl-proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(dtl-proto PROPERTIES CXX_VISIBILITY_PRESET default)
GR_LIBRARY_FOO(dtl-proto)
# Copy public headers for lib build ...
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/dtl/monitor.pb.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/monitor.pb.h ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/dtl
    DEPENDS dtl-proto
)
add_custom_target(
    copy_public_headers
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/dtl/monitor.pb.h
)
#add_dependencies(copy_public_headers dtl-proto)
# ... and install public headers to be used by other projects.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/monitor.pb.h DESTINATION include/gnuradio/dtl)

