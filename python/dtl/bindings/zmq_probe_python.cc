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
/* BINDTOOL_HEADER_FILE(zmq_probe.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(5815729a7400a5194e2406a5ede9eb9d)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dtl/zmq_probe.h>
// pydoc.h is automatically generated in the build directory
#include <zmq_probe_pydoc.h>

void bind_zmq_probe(py::module& m)
{

    using zmq_probe = ::gr::dtl::zmq_probe;


    py::class_<zmq_probe, gr::block, gr::basic_block, std::shared_ptr<zmq_probe>>(
        m, "zmq_probe", D(zmq_probe))

        .def(py::init(&zmq_probe::make),
             py::arg("address"),
             py::arg("probe_name"),
             py::arg("collection_name"),
             py::arg("bind") = true,
             D(zmq_probe, make))


        ;
}