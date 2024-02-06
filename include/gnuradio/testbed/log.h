/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_LOG_H
#define INCLUDED_DTL_LOG_H

#include <gnuradio/dtl/api.h>
#include <string>

namespace gr {
namespace dtl {

void DTL_API set_dtl_log_level(const std::string& level);


} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_LOG_H */
