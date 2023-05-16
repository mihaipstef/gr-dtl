/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_decoder.h"

#include <cstring>
#include "logger.h"
#include "repack.h"

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_decoder");

tb_decoder::tb_decoder(int max_tb_len, int max_cw_len): d_cw_buffers(2), d_payload(0)
{
    int max_len_bytes = max_tb_len / 8;   
    if (max_tb_len % 8) {
        ++max_len_bytes;
    }
    d_cw_buffers[0].resize(max_cw_len);
    d_cw_buffers[1].resize(max_cw_len);
    d_tb_buffers.resize(2);
    d_tb_buffers[0].reserve(max_tb_len);
    d_tb_buffers[1].reserve(max_len_bytes);
}

} // namespace dtl
} // namespace gr