/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_FRAME_FILE_STORE_H
#define INCLUDED_FRAME_FILE_STORE_H

#include <fstream>
#include <string>

namespace gr {
namespace dtl {

class frame_file_store
{
public:
    frame_file_store() = default;
    frame_file_store(std::string fname);
    void store(std::size_t payload_len, unsigned long count, const char* payload);

private:
    std::ofstream d_stream;
    unsigned d_frame_last_number;
    unsigned long long d_frame_long_count;
};

} // namespace dtl
} // namespace gr


#endif // INCLUDED_FRAME_FILE_STORE_H
