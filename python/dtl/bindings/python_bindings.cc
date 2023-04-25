/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
// The following comment block is used for
// gr_modtool to insert function prototypes
// Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_ofdm_adaptive_equalizer(py::module& m);
    void bind_ofdm_adaptive_frame_equalizer_vcvc(py::module& m);
    void bind_ofdm_adaptive_packet_header(py::module& m);
    void bind_ofdm_adaptive_utils(py::module& m);
    void bind_ofdm_adaptive_frame_pack_bb(py::module& m);
    void bind_ofdm_adaptive_chunks_to_symbols_bc(py::module& m);
    void bind_ofdm_adaptive_constellation_decoder_cb(py::module& m);
    void bind_ofdm_adaptive_frame_snr(py::module& m);
    void bind_ofdm_adaptive_feedback_decision(py::module& m);
    void bind_ofdm_adaptive_feedback_format(py::module& m);
    void bind_ofdm_adaptive_frame_bb(py::module& m);
    void bind_log(py::module& m);
    void bind_ofdm_adaptive_frame_detect_bb(py::module& m);
    void bind_zmq_msq_pub(py::module& m);
    void bind_ofdm_adaptive_constellation_metric_vcvf(py::module& m);
    void bind_ofdm_adaptive_fec_frame_bvb(py::module& m);
    void bind_ofdm_adaptive_fec_decoder(py::module& m);
    void bind_fec(py::module& m);
    void bind_ofdm_adaptive_frame_to_stream_vbb(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(dtl_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");

    /**************************************/
    // The following comment block is used for
    // gr_modtool to insert binding function calls
    // Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_ofdm_adaptive_equalizer(m);
    bind_ofdm_adaptive_frame_equalizer_vcvc(m);
    bind_ofdm_adaptive_packet_header(m);
    bind_ofdm_adaptive_utils(m);
    bind_ofdm_adaptive_frame_pack_bb(m);
    bind_ofdm_adaptive_chunks_to_symbols_bc(m);
    bind_ofdm_adaptive_constellation_decoder_cb(m);
    bind_ofdm_adaptive_frame_snr(m);
    bind_ofdm_adaptive_feedback_decision(m);
    bind_ofdm_adaptive_feedback_format(m);
    bind_ofdm_adaptive_frame_bb(m);
    bind_log(m);
    bind_ofdm_adaptive_frame_detect_bb(m);
    bind_zmq_msq_pub(m);
    bind_ofdm_adaptive_constellation_metric_vcvf(m);
    bind_ofdm_adaptive_fec_frame_bvb(m);
    bind_ofdm_adaptive_fec_decoder(m);
    bind_fec(m);
    bind_ofdm_adaptive_frame_to_stream_vbb(m);
    // ) END BINDING_FUNCTION_CALLS
}