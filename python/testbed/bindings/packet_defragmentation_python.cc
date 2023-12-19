#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/testbed/packet_defragmentation.h>

void bind_packet_defragmentation(py::module& m)
{

    using packet_defragmentation = ::gr::dtl::packet_defragmentation;


    py::class_<packet_defragmentation,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<packet_defragmentation>>(
        m, "packet_defragmentation", "Packet defragmentator")

        .def(py::init(&packet_defragmentation::make),
             py::arg("validator"),
             py::arg("len_key"),
             "Packet defragmentator constructor")


        ;
}
