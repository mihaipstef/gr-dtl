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
/* BINDTOOL_HEADER_FILE(ofdm_adaptive_utils.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(92ecda33719f9757d90244f05db9215a)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/ofdm_adaptive_utils.h>
// pydoc.h is automatically generated in the build directory
#include <ofdm_adaptive_utils_pydoc.h>

void bind_ofdm_adaptive_utils(py::module& m)
{


    py::enum_<::gr::dtl::constellation_type_t>(m, "constellation_type_t")
        .value("UNKNOWN", ::gr::dtl::constellation_type_t::UNKNOWN) // 0
        .value("BPSK", ::gr::dtl::constellation_type_t::BPSK)       // 1
        .value("QPSK", ::gr::dtl::constellation_type_t::QPSK)       // 2
        .value("PSK8", ::gr::dtl::constellation_type_t::PSK8)       // 3
        .value("QAM16", ::gr::dtl::constellation_type_t::QAM16)     // 4
        .export_values();

    py::implicitly_convertible<int, ::gr::dtl::constellation_type_t>();


    m.def("get_constellation",
          &::gr::dtl::get_constellation,
          py::arg("constellation_type"),
          D(get_constellation));


    m.def("get_bits_per_symbol",
          &::gr::dtl::get_bits_per_symbol,
          py::arg("constellation"),
          D(get_bits_per_symbol));


    m.def("get_max_bps", &::gr::dtl::get_max_bps, py::arg("cnsts"), D(get_max_bps));


    m.def("create_constellation",
          &::gr::dtl::create_constellation,
          py::arg("constellation"),
          D(create_constellation));


    m.def("find_constellation_type",
          &::gr::dtl::find_constellation_type,
          py::arg("tags"),
          D(find_constellation_type));


    m.def("get_constellation_type",
          &::gr::dtl::get_constellation_type,
          py::arg("tag"),
          D(get_constellation_type));


    m.def("find_constellation_tag",
          &::gr::dtl::find_constellation_tag,
          py::arg("tags"),
          D(find_constellation_tag));


    m.def("find_tag", &::gr::dtl::find_tag, py::arg("tags"), py::arg("key"), D(find_tag));


    m.def("get_constellation_tag_key",
          &::gr::dtl::get_constellation_tag_key,
          D(get_constellation_tag_key));


    m.def("estimated_snr_tag_key",
          &::gr::dtl::estimated_snr_tag_key,
          D(estimated_snr_tag_key));


    m.def("noise_tag_key", &::gr::dtl::noise_tag_key, D(noise_tag_key));


    m.def("feedback_constellation_key",
          &::gr::dtl::feedback_constellation_key,
          D(feedback_constellation_key));


    m.def("payload_length_key", &::gr::dtl::payload_length_key, D(payload_length_key));


    m.def("fec_key", &::gr::dtl::fec_key, D(fec_key));


    m.def("fec_feedback_key", &::gr::dtl::fec_feedback_key, D(fec_feedback_key));


    m.def("fec_tb_key", &::gr::dtl::fec_tb_key, D(fec_tb_key));


    m.def("fec_offset_key", &::gr::dtl::fec_offset_key, D(fec_offset_key));


    m.def("fec_tb_payload_key", &::gr::dtl::fec_tb_payload_key, D(fec_tb_payload_key));


    m.def("fec_tb_len_key", &::gr::dtl::fec_tb_len_key, D(fec_tb_len_key));


    m.def("fec_tb_index_key", &::gr::dtl::fec_tb_index_key, D(fec_tb_index_key));
}
