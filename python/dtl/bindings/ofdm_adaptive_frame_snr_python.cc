/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */


#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/digital/mpsk_snr_est.h>
#include <gnuradio/dtl/ofdm_adaptive_frame_snr.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_frame_snr_pydoc.h>


template <class T>
void bind_ofdm_adaptive_frame_snr_template(py::module& m, const char* classname)
{
    using ofdm_adaptive_frame_snr_base = gr::dtl::ofdm_adaptive_frame_snr_base;
    using frame_snr = gr::dtl::ofdm_adaptive_frame_snr<T>;

    py::class_<frame_snr,
               ofdm_adaptive_frame_snr_base,
               std::shared_ptr<frame_snr>>(m, classname)
          .def(py::init<frame_snr const&>(),
               py::arg("arg0"))
          .def(py::init<double>(),
               py::arg("alpha"))
        .def("update",
             &frame_snr::update,
             py::arg("noutput_items"),
             py::arg("input"))
        .def("reset",
             &frame_snr::reset)
        .def("snr",
             &frame_snr::snr)
        .def("noise",
             &frame_snr::snr);
}



void bind_ofdm_adaptive_frame_snr(py::module& m)
{

    using ofdm_adaptive_frame_snr_base = ::gr::dtl::ofdm_adaptive_frame_snr_base;


    py::class_<ofdm_adaptive_frame_snr_base,
               std::shared_ptr<ofdm_adaptive_frame_snr_base>>(
        m, "ofdm_adaptive_frame_snr_base")

     //    .def(py::init<>(),
     //         D(ofdm_adaptive_frame_snr_base, ofdm_adaptive_frame_snr_base, 0))
     //    .def(py::init<gr::dtl::ofdm_adaptive_frame_snr_base const&>(),
     //         py::arg("arg0"),
     //         D(ofdm_adaptive_frame_snr_base, ofdm_adaptive_frame_snr_base, 1))


        .def("update",
             &ofdm_adaptive_frame_snr_base::update,
             py::arg("noutput_items"),
             py::arg("input"))


        .def("reset",
             &ofdm_adaptive_frame_snr_base::reset)


        .def("snr",
             &ofdm_adaptive_frame_snr_base::snr)

        .def("noise",
             &ofdm_adaptive_frame_snr_base::snr)

        ;

     bind_ofdm_adaptive_frame_snr_template<gr::digital::mpsk_snr_est_simple>(m, "ofdm_adaptive_frame_snr_simple");

}
