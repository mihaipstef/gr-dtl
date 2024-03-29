
/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <chrono>
#include <gnuradio/dtl/api.h>
#include <gnuradio/testbed/monitor_msg.h>


namespace gr {
namespace dtl {


uint64_t DTL_API system_ts() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}


} // dtl
} // gr