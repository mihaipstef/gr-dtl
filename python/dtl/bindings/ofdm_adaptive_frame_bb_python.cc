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
/* BINDTOOL_HEADER_FILE(ofdm_adaptive_frame_bb.h) */
/* BINDTOOL_HEADER_FILE_HASH(1cc6ac645aa0909fec84c4df4b416162)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/ofdm_adaptive_frame_bb.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_frame_bb_pydoc.h>

void bind_ofdm_adaptive_frame_bb(py::module& m)
{

    using ofdm_adaptive_frame_bb = ::gr::dtl::ofdm_adaptive_frame_bb;


    py::class_<ofdm_adaptive_frame_bb,
               gr::block,
               gr::basic_block,
               std::shared_ptr<ofdm_adaptive_frame_bb>>(
        m, "ofdm_adaptive_frame_bb", D(ofdm_adaptive_frame_bb))

        .def(py::init(&ofdm_adaptive_frame_bb::make),
             py::arg("len_tag_key"),
             py::arg("frame_len"),
             py::arg("n_payload_carriers"),
             D(ofdm_adaptive_frame_bb, make))


        .def("set_constellation",
             &ofdm_adaptive_frame_bb::set_constellation,
             py::arg("constellation"),
             D(ofdm_adaptive_frame_bb, set_constellation))

        ;
}