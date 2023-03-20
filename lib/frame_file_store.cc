/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frame_file_store.h"
#include "logger.h"
#include <math.h>

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("frame_file_store")

const int MAX_HEADER_FRAME_NUMBER = 4095; // 2 ^ 12 - 1 (12 bits)
const int MAX_SKIP = 3;

frame_file_store::frame_file_store(string fname)
    : d_stream(fname, ios::binary),
      d_frame_last_number(0),
      d_frame_long_count(0),
      d_skip_count(MAX_SKIP)
{
}

void frame_file_store::store(size_t payload_len, unsigned long count, const char* payload)
{

    if (abs(static_cast<int>(count - d_frame_last_number)) >= 5 && d_skip_count) {
        --d_skip_count;
        return;
    }

    int long_count_increment = 0;
    if (count >= d_frame_last_number) {
        long_count_increment = (count - d_frame_last_number);
    } else {
        long_count_increment = (MAX_HEADER_FRAME_NUMBER - d_frame_last_number) + count + 1;
    }

    if (long_count_increment >= 5 && d_skip_count) {
        --d_skip_count;
        return;
    }
    d_skip_count = MAX_SKIP;

    // Update long count
    d_frame_long_count += long_count_increment;
    d_frame_last_number = count;

    DTL_LOG_DEBUG("no={}, long_no={}, len={}", count, d_frame_long_count, payload_len);
    if (d_stream.is_open()) {
        d_stream.write(reinterpret_cast<char*>(&payload_len), 4);
        d_stream.write(reinterpret_cast<char*>(&d_frame_long_count), 8);
        d_stream.write(payload, payload_len);
    }
}


} // namespace dtl
} // namespace gr