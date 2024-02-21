#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/testbed/phy_converge.h>

void bind_convergence_layer(py::module& m)
{

    using from_phy = ::gr::dtl::from_phy;
    using to_phy = ::gr::dtl::to_phy;
    using transported_protocol_t = ::gr::dtl::transported_protocol_t;

    py::enum_<transported_protocol_t>(m, "transported_protocol_t")
        .value("IPV4_ONLY", transported_protocol_t::IPV4_ONLY)
        .value("ETHER_IPV4_ONLY", transported_protocol_t::ETHER_IPV4_ONLY)
        .value("MODIFIED_ETHER", transported_protocol_t::MODIFIED_ETHER)
        .export_values();

    py::implicitly_convertible<int, transported_protocol_t>();

    py::class_<from_phy,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<from_phy>>(m, "from_phy", "Adapt PHY data to upper layer")

        .def(py::init(&from_phy::make),
             py::arg("protocol"),
             py::arg("validator"),
             py::arg("len_key"),
             "PHY to upper layer block constructor");

    py::class_<to_phy,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<to_phy>>(m, "to_phy", "Adapt to upper layer to PHY")

        .def(py::init(&to_phy::make),
             py::arg("protocol"),
             py::arg("len_key"),
             py::arg("bpb"),
             "Upper layer to PHY block constructor");
}
