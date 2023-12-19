#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/testbed/packet_validator.h>

void bind_packet_validator(py::module& m)
{

    using packet_validator = ::gr::dtl::packet_validator;
    using ip_validator = ::gr::dtl::ip_validator;
    using ethernet_validator = ::gr::dtl::ethernet_validator;


    py::class_<packet_validator, std::shared_ptr<packet_validator>>(
        m, "packet_validator", "Packet validator")

        .def("valid",
             &packet_validator::valid,
             py::arg("buf"),
             py::arg("len"),
             "Check if packet is valid")

        ;


    py::class_<ip_validator, gr::dtl::packet_validator, std::shared_ptr<ip_validator>>(
        m, "ip_validator", "L3 packet validator")

        .def(py::init<std::string const&>(),
             py::arg("src_addr"),
             "L3 packet validator")
        .def(py::init<gr::dtl::ip_validator const&>(),
             py::arg("arg0"),
             "L3 packet validator")


        .def("valid",
             &ip_validator::valid,
             py::arg("buf"),
             py::arg("len"),
             "Check if L3 packet is valid")

        ;


    py::class_<ethernet_validator,
               gr::dtl::packet_validator,
               std::shared_ptr<ethernet_validator>>(
        m, "ethernet_validator", "L2 packet validator")

        .def(py::init<std::string const&>(),
             py::arg("dst_addr"),
             "L2 packet validator")
        .def(py::init<gr::dtl::ethernet_validator const&>(),
             py::arg("arg0"),
             "L2 packet validator")


        .def("valid",
             &ethernet_validator::valid,
             py::arg("buf"),
             py::arg("len"),
             "Check if L2 packet is valid")

        ;
}
