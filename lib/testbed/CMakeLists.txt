include(GrPlatform) #define LIB_SUFFIX

find_package(ZeroMQ REQUIRED)
find_package(Protobuf REQUIRED)
include(FindProtobuf)


list (APPEND proto_schema_files
    monitor.proto)

include_directories(${PROTOBUF_INCLUDE_DIR})
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${proto_schema_files})
add_library(monitoring-proto SHARED ${PROTO_SRC})
set_target_properties(monitoring-proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(monitoring-proto PROPERTIES CXX_VISIBILITY_PRESET default)
GR_LIBRARY_FOO(monitoring-proto)
# Copy public headers for lib build ...
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/testbed/monitor.pb.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/monitor.pb.h ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/testbed
    DEPENDS monitoring-proto
)
add_custom_target(
    copy_monitoring_proto_headers
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/testbed/monitor.pb.h
)
#add_dependencies(copy_public_headers dtl-proto)
# ... and install public headers to be used by other projects.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../../include/gnuradio/testbed/monitor.pb.h DESTINATION include/gnuradio/testbed)

list(APPEND monitoring_sources
    monitor_probe_impl.cc
    monitor_msg.cc
    monitor_parser.cc
    monitor_registry.cc
    packet_validator.cc
    from_phy_impl.cc
    to_phy_impl.cc
    logger.cc)

set(monitoring_sources "${monitoring_sources}" PARENT_SCOPE)
if(NOT monitoring_sources)
    MESSAGE(STATUS "No C++ sources... skipping lib/")
    return()
endif(NOT monitoring_sources)


add_library(dtl-testbed SHARED ${monitoring_sources})
set_target_properties(dtl-testbed PROPERTIES CXX_VISIBILITY_PRESET default)
target_link_libraries(dtl-testbed gnuradio::gnuradio-runtime)
target_link_libraries(dtl-testbed ZeroMQ::ZeroMQ)
target_link_libraries(dtl-testbed monitoring-proto ${Protobuf_LIBRARIES})
add_dependencies(dtl-testbed copy_monitoring_proto_headers)


target_include_directories(dtl-testbed
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    PUBLIC $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../include
)

set_target_properties(dtl-testbed PROPERTIES DEFINE_SYMBOL "gnuradio_monitoring_EXPORTS")
target_compile_definitions(dtl-testbed PRIVATE DTL_LOGGING_ENABLE=${DTL_LOGGING_ENABLE})

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(dtl-testbed)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")