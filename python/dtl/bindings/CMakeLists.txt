# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT dtl_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT dtl_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND dtl_python_files
    ofdm_adaptive_equalizer_python.cc
    ofdm_adaptive_frame_equalizer_vcvc_python.cc
    ofdm_adaptive_packet_header_python.cc
    ofdm_adaptive_utils_python.cc
    ofdm_adaptive_repack_bits_bb_python.cc
    ofdm_adaptive_chunks_to_symbols_bc_python.cc
    ofdm_adaptive_constellation_decoder_cb_python.cc
    ofdm_adaptive_frame_snr_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(dtl
   ../../..
   gr::dtl
   "${dtl_python_files}")

# copy in bindings .so file for use in QA test module
add_custom_target(
  copy_bindings_for_tests ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/*.so"
    ${CMAKE_BINARY_DIR}/test_modules/gnuradio/dtl/
  DEPENDS dtl_python)

install(TARGETS dtl_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/dtl COMPONENT pythonapi)
