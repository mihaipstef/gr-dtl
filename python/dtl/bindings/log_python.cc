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
/* BINDTOOL_HEADER_FILE(log.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(9d2fb206bbbfeb3316190057c1c038ca)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/log.h>
// pydoc.h is automatically generated in the build directory
#include <log_pydoc.h>

void bind_log(py::module& m)
{


    m.def("set_dtl_log_level",
          &::gr::dtl::set_dtl_log_level,
          py::arg("level"),
          D(set_dtl_log_level));
}
