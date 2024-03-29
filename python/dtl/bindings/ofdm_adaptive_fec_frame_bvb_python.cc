/*
 * Copyright 2023 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(ofdm_adaptive_fec_frame_bvb.h) */
/* BINDTOOL_HEADER_FILE_HASH(02015b4165d9de8b0efb577b1897bd2f)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/ofdm_adaptive_fec_frame_bvb.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_fec_frame_bvb_pydoc.h>

void bind_ofdm_adaptive_fec_frame_bvb(py::module& m)
{

    using ofdm_adaptive_fec_frame_bvb = ::gr::dtl::ofdm_adaptive_fec_frame_bvb;


    py::class_<ofdm_adaptive_fec_frame_bvb,
               gr::block,
               gr::basic_block,
               std::shared_ptr<ofdm_adaptive_fec_frame_bvb>>(
        m, "ofdm_adaptive_fec_frame_bvb", D(ofdm_adaptive_fec_frame_bvb))

        .def(py::init(&ofdm_adaptive_fec_frame_bvb::make),
             py::arg("encoders"),
             py::arg("frame_capacity"),
             py::arg("frame_rate"),
             py::arg("max_bps"),
             py::arg("max_empty_frames"),
             py::arg("len_key"),
             D(ofdm_adaptive_fec_frame_bvb, make))


        .def("process_feedback",
             &ofdm_adaptive_fec_frame_bvb::process_feedback,
             py::arg("feedback"),
             D(ofdm_adaptive_fec_frame_bvb, process_feedback))

        ;
}
