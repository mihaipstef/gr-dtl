/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_decoder.h"

#include <cstring>
#include "fec_utils.h"
#include "logger.h"
#include "repack.h"

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_decoder");

tb_decoder::tb_decoder(int max_tb_len): d_payload(0), d_tb_number(-1), d_buf_idx(0)
{
    d_tb_buffers.resize(3);
    d_tb_buffers[RCV_BUF].reserve(max_tb_len);
    d_tb_buffers[FULL_BUF].reserve(max_tb_len);
    d_data_buffer.reserve(max_tb_len);
}

bool tb_decoder::process_frame(const float* in, int payload_len, fec_info_t::sptr fec_info)
{
    bool data_ready = false;

    if (!fec_info) {
        DTL_LOG_DEBUG("process_frame: fec info fail");
        return false;
    }

    // If frame is part of the current TB
    DTL_LOG_DEBUG("process_frame: current_no={}, no={}, offset={}", d_tb_number, fec_info->d_tb_number, fec_info->d_tb_offset);
    if (fec_info->d_tb_number == d_tb_number) {
        memcpy(&d_tb_buffers[RCV_BUF][d_buf_idx], in, payload_len);
        d_buf_idx += payload_len;
    // If frame is part of a new TB
    } else {
         // Fill current TB buffer and decode
        memcpy(&d_tb_buffers[RCV_BUF][d_buf_idx], in, fec_info->d_tb_offset);

        // Decode
        if (d_fec_info) {
            decode();
        }

        // Start new TB buffer
        d_fec_info = fec_info;
        d_tb_number = fec_info->d_tb_number;
        d_tb_payload_len = fec_info->d_tb_payload_len;
        d_buf_idx = 0;
        memcpy(&d_tb_buffers[RCV_BUF][d_buf_idx], in, payload_len - fec_info->d_tb_offset);
        data_ready = true;
    }
    return data_ready;
}

void tb_decoder::decode()
{
    static float SHORTENED_VALUE = 0;

    int payload_len = d_fec_info->d_tb_payload_len;
    int ncws = d_fec_info->no_of_cws();
    int n = d_fec_info->get_n();
    int k = d_fec_info->get_k();
    int ncheck = n - k;
    int n_iterations;

    d_data_buffer.resize(d_fec_info->d_tb_payload_len);

    for (int i=0, j=0; i < ncws; ++i) {
        int k_ = payload_len / (ncws - i);
        if (payload_len % (ncws - i)) {
            ++k_;
        }
        payload_len -= k_;

        // copy check bits
        memcpy(&d_tb_buffers[FULL_BUF][i*n], &d_tb_buffers[RCV_BUF][j], ncheck);
        j += ncheck;
        memset(&d_tb_buffers[FULL_BUF][i*n + ncheck], SHORTENED_VALUE, k-k_);
        memcpy(&d_tb_buffers[FULL_BUF][(i+1)*n - k_], &d_tb_buffers[RCV_BUF][j], k_);
        j += k_;
        d_fec_info->d_dec->decode(&d_tb_buffers[FULL_BUF][i*n], &n_iterations, &d_data_buffer[i*k_]);
    }
}


const std::vector<unsigned char>& tb_decoder::data() const {
    return d_data_buffer;
}

} // namespace dtl
} // namespace gr