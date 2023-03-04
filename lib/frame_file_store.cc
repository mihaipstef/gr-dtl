/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frame_file_store.h"

namespace gr {
namespace dtl {

using namespace std;

frame_file_store::frame_file_store(string fname) : d_stream(fname, ios::binary) {}

void frame_file_store::store(size_t payload_len,
                             unsigned long count,
                             const char* payload)
{
    if(d_stream.is_open()) {
        d_stream.write(reinterpret_cast<char*>(payload_len), sizeof(unsigned int));
        d_stream.write(reinterpret_cast<char*>(count), sizeof(unsigned long));
        d_stream.write(payload, payload_len);
    }
}


} // namespace dtl
} // namespace gr