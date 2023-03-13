/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frame_file_store.h"
#include "logger.h"

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("frame_file_store")

frame_file_store::frame_file_store(string fname) : d_stream(fname, ios::binary), d_frame_last_number(0), d_frame_long_count() {}

void frame_file_store::store(size_t payload_len,
                             unsigned long count,
                             const char* payload)
{
    if (count >= d_frame_last_number) {
        d_frame_long_count += (count - d_frame_last_number);
    } else {
        d_frame_long_count += count + 1;
    }
    d_frame_last_number = count;
    DTL_LOG_DEBUG("no={}, long_no={}, len={}", count, d_frame_long_count, payload_len);
    if(d_stream.is_open()) {
        d_stream.write(reinterpret_cast<char*>(&payload_len), 4);
        d_stream.write(reinterpret_cast<char*>(&d_frame_long_count), 8);
        d_stream.write(payload, payload_len);
    }
}


} // namespace dtl
} // namespace gr