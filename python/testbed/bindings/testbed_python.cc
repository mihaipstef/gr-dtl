/*
 * Copyright 2023 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */


#include <gnuradio/testbed/monitor_parser.h>
#include <gnuradio/testbed/monitor_registry.h>
#include <gnuradio/testbed/monitor_probe.h>
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>


namespace py = pybind11;


PYBIND11_MAKE_OPAQUE(::gr::dtl::msg_t);
PYBIND11_MAKE_OPAQUE(::gr::dtl::msg_dict_t);


std::unique_ptr<gr::dtl::parse_result> parse_msg(py::buffer data, size_t size)
{
    py::buffer_info info = data.request();
    auto r = std::make_unique<gr::dtl::parse_result>();
    r->encoding = ::gr::dtl::parse((uint8_t*)info.ptr, size, *r);
    return std::move(r);
}


void bind_monitoring(py::module& m)
{

    using message_sender_base = ::gr::dtl::message_sender_base;
    using message_sender = ::gr::dtl::message_sender;
    using monitor_probe = ::gr::dtl::monitor_probe;

    py::bind_map<::gr::dtl::msg_dict_t>(m, "msg_dict_t");

    py::enum_<::gr::dtl::msg_encoding_t>(m, "msg_encoding_t")
        .value("UNKNOWN", ::gr::dtl::msg_encoding_t::UNKNOWN)
        .value("PROTO", ::gr::dtl::msg_encoding_t::PROTO)
        .value("PROTO_IN_BLOB", ::gr::dtl::msg_encoding_t::PROTO_IN_BLOB)
        .value("PMT", ::gr::dtl::msg_encoding_t::PMT)
        .export_values();

    py::implicitly_convertible<int, ::gr::dtl::msg_encoding_t>();

    py::class_<gr::dtl::parse_result, std::unique_ptr<gr::dtl::parse_result>>(
        m, "parse_result", "parse_result")
        .def(py::init<>())
        .def_readonly("encoding", &gr::dtl::parse_result::encoding)
        .def("get_dict", &gr::dtl::parse_result::dict, "get", py::return_value_policy::reference_internal)
        .def("get_pmt", &gr::dtl::parse_result::pmt, "get", py::return_value_policy::reference_internal);

    m.def("parse_msg",
          &parse_msg,
          py::arg("data"),
          py::arg("size"),
          "Parse monitor message");

    py::class_<message_sender_base, std::shared_ptr<message_sender_base>>(
        m, "message_sender_base", "Message sender base class")
        .def("send",
             &message_sender_base::send,
             py::arg("msg"),
             "Send method")

        ;


    py::class_<message_sender,
               gr::dtl::message_sender_base,
               std::shared_ptr<message_sender>>(m, "message_sender", "Message sender")

        .def(py::init(&message_sender::make),
             py::arg("address"),
             py::arg("bind"),
             "Message sender constructor")


        ;


    py::class_<monitor_probe, gr::block, gr::basic_block, std::shared_ptr<monitor_probe>>(
        m, "monitor_probe", "Monitor probe")

        .def(py::init(&monitor_probe::make),
             py::arg("name"),
             py::arg("sender"),
             "Monitor probe constructor")


        .def("monitor_msg_handler",
             &monitor_probe::monitor_msg_handler,
             py::arg("msg"),
             "Message handler")

        ;
}


void* init_numpy()
{
    import_array();
    return NULL;
}


void bind_packet_validator(py::module& m);
void bind_convergence_layer(py::module& m);

PYBIND11_MODULE(testbed_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();
    py::module::import("gnuradio.gr");
    bind_monitoring(m);
    bind_packet_validator(m);
    bind_convergence_layer(m);
}