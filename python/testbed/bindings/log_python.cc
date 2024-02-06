#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/testbed/log.h>

void bind_log(py::module& m)
{


    m.def("set_dtl_log_level",
          &::gr::dtl::set_dtl_log_level,
          py::arg("level"),
          "Set log level");
}
