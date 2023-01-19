/*
 * Copyright 2022 Free Software Foundation, Inc.
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
/* BINDTOOL_HEADER_FILE(ofdm_adaptive_frame_equalizer_vcvc.h) */
/* BINDTOOL_HEADER_FILE_HASH(3a3a9e1fe45b843c91ad0ae6031097c4)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/ofdm_adaptive_frame_equalizer_vcvc.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_frame_equalizer_vcvc_pydoc.h>

void bind_ofdm_adaptive_frame_equalizer_vcvc(py::module& m)
{

    using ofdm_adaptive_frame_equalizer_vcvc =
        ::gr::dtl::ofdm_adaptive_frame_equalizer_vcvc;


    py::class_<ofdm_adaptive_frame_equalizer_vcvc,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<ofdm_adaptive_frame_equalizer_vcvc>>(
        m, "ofdm_adaptive_frame_equalizer_vcvc", D(ofdm_adaptive_frame_equalizer_vcvc))

        .def(py::init(&ofdm_adaptive_frame_equalizer_vcvc::make),
             py::arg("equalizer"),
             py::arg("feedback_decision"),
             py::arg("cp_len"),
             py::arg("tsb_key") = "frame_len",
             py::arg("propagate_channel_state") = false,
             py::arg("propagate_feedback_tags") = false,
             py::arg("fixed_frame_len") = 0,
             D(ofdm_adaptive_frame_equalizer_vcvc, make))


        ;
}
