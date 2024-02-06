/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_encoder.h"

#include <gnuradio/testbed/logger.h>
#include "repack.h"
#include <cstring>

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_encoder");

tb_encoder::tb_encoder(int max_tb_len, int max_cw_len)
    : d_cw_buffers(2), d_payload(0)
{
    d_cw_buffers[0].resize(max_cw_len);
    d_cw_buffers[1].resize(max_cw_len);
    d_tb_buffers.resize(2);
    d_tb_buffers[0].reserve(max_tb_len);
    d_tb_buffers[1].reserve(max_tb_len);
}


int tb_encoder::encode(const unsigned char* in,
                       int len,
                       fec_enc::sptr enc,
                       int current_tb_len)
{

    int read_index = 0;

    d_tb_buffers[0].clear();
    d_tb_buffers[1].clear();
    d_payload = 0;
    d_buf_idx = 0;
    int ncheck = enc->get_n() - enc->get_k();

    DTL_LOG_DEBUG("encode: ncws={}", current_tb_len);

    for (int i = 0; i < current_tb_len; ++i) {
        int k_new = (len - d_payload) / (current_tb_len - i);
        if ((len - d_payload) % (current_tb_len - i)) {
            ++k_new;
        }
        d_payload += k_new;

        // copy K' bits from input into the buffer
        memset(&d_cw_buffers[0][0], 0, enc->get_n());
        memcpy(&d_cw_buffers[0][0], &in[read_index], k_new);

        // calculate the codeword
        enc->encode(&d_cw_buffers[0][0], enc->get_k(), &d_cw_buffers[1][0]);

        read_index += k_new;

        // Move cw to the TB buffer
        copy(&d_cw_buffers[1][0],
             &d_cw_buffers[1][ncheck],
             back_inserter(d_tb_buffers[0]));
        copy(&d_cw_buffers[1][ncheck],
             &d_cw_buffers[1][ncheck + k_new],
             back_inserter(d_tb_buffers[0]));
    }

    return d_tb_buffers[0].size();
}

bool tb_encoder::ready() const
{
    return d_tb_buffers[0].size() == 0 || d_tb_buffers[0].size() == d_buf_idx;
}

int tb_encoder::remaining_buf_size()
{
    return static_cast<int>(d_tb_buffers[0].size() - d_buf_idx);
}

int tb_encoder::size() { return static_cast<int>(d_tb_buffers[0].size()); }

int tb_encoder::buf_out(unsigned char* out, int len, int bps)
{
    repack repacker(1, bps);
    int syms = repacker.repack_lsb_first(&d_tb_buffers[0][d_buf_idx], len, out);
    d_buf_idx += len;
    DTL_LOG_DEBUG("buf_out: idx={}, size={}, n_syms={}, len={}",
                  d_buf_idx,
                  d_tb_buffers[0].size(),
                  syms,
                  len);
    return syms;
}

int tb_encoder::buf_payload() { return d_payload; }

} // namespace dtl
} // namespace gr