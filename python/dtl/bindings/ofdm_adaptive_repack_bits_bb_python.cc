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
/* BINDTOOL_HEADER_FILE(ofdm_adaptive_repack_bits_bb.h) */
/* BINDTOOL_HEADER_FILE_HASH(bef943917b4faae85d091180e0ae04f0)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/ofdm_adaptive_repack_bits_bb.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_repack_bits_bb_pydoc.h>

void bind_ofdm_adaptive_repack_bits_bb(py::module& m)
{

    using ofdm_adaptive_repack_bits_bb = ::gr::dtl::ofdm_adaptive_repack_bits_bb;


    py::class_<ofdm_adaptive_repack_bits_bb,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<ofdm_adaptive_repack_bits_bb>>(
        m, "ofdm_adaptive_repack_bits_bb", D(ofdm_adaptive_repack_bits_bb))

        .def(py::init(&ofdm_adaptive_repack_bits_bb::make),
             py::arg("tsb_tag_key") = "",
             py::arg("unpack") = true,
             py::arg("endianness") = ::gr::endianness_t::GR_LSB_FIRST,
             D(ofdm_adaptive_repack_bits_bb, make))


        ;
}
