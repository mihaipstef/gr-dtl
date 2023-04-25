/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_encoder.h"

#include <cstring>
#include "logger.h"
#include "repack.h"

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_encoder");

tb_encoder::tb_encoder(int max_tb_len, int max_cw_len): d_cw_buffers(2), d_payload(0)
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


int tb_encoder::encode(const unsigned char* in, int len, fec_enc::sptr enc, int current_tb_len)
{

    int read_index = 0;

    memset(&d_cw_buffers[0][0], 0, enc->get_n());
    d_tb_buffers[0].clear();
    d_tb_buffers[1].clear();
    d_payload = 0;
    d_buf_idx = 0;

    for (int i = 0; i < current_tb_len; ++i) {
        int k_new = (len - d_payload) / (current_tb_len-i);
        if ((len - d_payload) % (current_tb_len - i)) {
            ++k_new;
        }
        d_payload += k_new;
        //d_tb_systematic_bits += k_new;
        // copy K' bits from input into the buffer
        memcpy(&d_cw_buffers[0][0], &in[read_index], k_new);
        //DTL_LOG_BYTES("cw buffer 0", &d_cw_buffers[0][0], k_new);
        // calculate the codeword
        enc->encode(&d_cw_buffers[0][0], enc->get_k(), &d_cw_buffers[1][0]);
        //DTL_LOG_BYTES("cw buffer 1", &d_cw_buffers[1][0],  enc->get_n());

        read_index += k_new;
        //++d_cw_count;
        // Move cw to the TB buffer
        copy(&d_cw_buffers[1][0], &d_cw_buffers[1][k_new], back_inserter(d_tb_buffers[0]));
        copy(&d_cw_buffers[1][enc->get_k()], &d_cw_buffers[1][enc->get_n()], back_inserter(d_tb_buffers[0]));
    }
    //DTL_LOG_BYTES("tb buffer", &d_tb_buffers[0][0], d_tb_buffers[0].size());
    return d_tb_buffers[0].size();
}

bool tb_encoder::ready()
{
    return d_tb_buffers[0].size() == 0 || d_tb_buffers[0].size() == d_buf_idx;
}

int tb_encoder::remaining_buf_size()
{
    return static_cast<int>(d_tb_buffers[0].size() - d_buf_idx);
}

int tb_encoder::size()
{
    return static_cast<int>(d_tb_buffers[0].size());
}

int tb_encoder::buf_out(unsigned char* out, int len, int bps)
{
    repack repacker(1, bps);
    int syms = repacker.repack_lsb_first(&d_tb_buffers[0][d_buf_idx], len, out, false);
    d_buf_idx += len;
    DTL_LOG_DEBUG("buf_out: idx={}, size={}", d_buf_idx, d_tb_buffers[0].size());
    return syms;
}

int tb_encoder::buf_payload()
{
    return d_payload;
}

} // namespace dtl
} // namespace gr