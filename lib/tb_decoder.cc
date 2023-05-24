/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_decoder.h"

#include "fec_utils.h"
#include "logger.h"
#include "repack.h"
#include <cstring>

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_decoder");

tb_decoder::tb_decoder(int max_tb_len)
    : d_payload(0), d_tb_number(-1), d_buf_idx(0) //, d_fec_info(nullptr)
{
    d_tb_buffers.resize(2);
    d_tb_buffers[RCV_BUF].reserve(2 * max_tb_len);
    d_tb_buffers[FULL_BUF].reserve(2 * max_tb_len);
    d_data_buffer.reserve(2 * max_tb_len);
    d_tb_buffers[RCV_BUF].clear();
}

bool tb_decoder::process_frame(const float* in,
                               int payload_len,
                               fec_info_t::sptr fec_info)
{
    bool data_ready = false;

    if (!fec_info) {
        DTL_LOG_DEBUG("process_frame: fec info fail");
        return false;
    }

    // If frame is part of the current TB
    DTL_LOG_DEBUG("process_frame: current_no={}, no={}, offset={}, frame_len={}",
                  d_tb_number,
                  fec_info->d_tb_number,
                  fec_info->d_tb_offset,
                  payload_len);

    DTL_LOG_BUFFER("process frame input", in, payload_len);

    if (fec_info->d_tb_number == d_tb_number) {
        if (d_buf_idx + payload_len <= d_tb_buffers[RCV_BUF].capacity()) {
            copy(in, in + payload_len, back_inserter(d_tb_buffers[RCV_BUF]));
            d_buf_idx += payload_len;
        }
        // If frame is part of a new TB
    } else {
        // Fill current TB buffer and decode
        if (fec_info->d_tb_offset > 0) {
            copy(in, in + fec_info->d_tb_offset, back_inserter(d_tb_buffers[RCV_BUF]));
        }

        // Decode
        if (d_tb_buffers[RCV_BUF].size() > 0) {
            DTL_LOG_VEC("before decode", d_tb_buffers[RCV_BUF]);
            int tb_len = compute_tb_len(d_fec_info->get_n(), payload_len);
            decode(tb_len);
        }

        // Start new TB buffer
        d_fec_info = fec_info;
        d_tb_number = d_fec_info->d_tb_number;
        d_tb_payload_len = d_fec_info->d_tb_payload_len;
        d_buf_idx = 0;
        d_tb_buffers[RCV_BUF].clear();
        // memcpy(
        //     &d_tb_buffers[RCV_BUF][d_buf_idx], in, sizeof(float) * (payload_len - fec_info->d_tb_offset));
        copy(
            in, in + (payload_len - fec_info->d_tb_offset), back_inserter(d_tb_buffers[RCV_BUF]));
        d_buf_idx += payload_len - fec_info->d_tb_offset;
        data_ready = true;
    }
    DTL_LOG_VEC("process frame rcv buffer", d_tb_buffers[RCV_BUF]);
    return data_ready;
}

void tb_decoder::decode(int tb_len)
{
    static float SHORTENED_VALUE = -2;


    int payload_len = d_fec_info->d_tb_payload_len;
    int n = d_fec_info->get_n();
    int k = d_fec_info->get_k();
    int ncheck = n - k;
    int n_iterations = 0;

    d_data_buffer.resize(d_fec_info->d_tb_payload_len);
    d_tb_buffers[FULL_BUF].clear();
    fill(&d_tb_buffers[FULL_BUF][0], &d_tb_buffers[FULL_BUF][tb_len * n], SHORTENED_VALUE);
    DTL_LOG_DEBUG("decode: ncws={}, n_it={}, sz={}, tb_payload_len={}", tb_len, n_iterations, d_tb_buffers[FULL_BUF].size(), payload_len);
    for (int i = 0, j = 0; i < tb_len; ++i) {
        int k_ = payload_len / (tb_len - i);
        if (payload_len % (tb_len - i)) {
            ++k_;
        }
        payload_len -= k_;

        // copy check bits
        //copy(&d_tb_buffers[FULL_BUF][i * n], &d_tb_buffers[RCV_BUF][j], ncheck);
        copy(&d_tb_buffers[RCV_BUF][j], &d_tb_buffers[RCV_BUF][j+ncheck], d_tb_buffers[FULL_BUF].begin()+i*n);
        j += ncheck;        
        //memcpy(&d_tb_buffers[FULL_BUF][(i + 1) * n - k_], &d_tb_buffers[RCV_BUF][j], k_);
        copy(&d_tb_buffers[RCV_BUF][j], &d_tb_buffers[RCV_BUF][j+k_], d_tb_buffers[FULL_BUF].begin()+((i + 1) * n - k_));

        j += k_;
        d_fec_info->d_dec->decode(
            &d_tb_buffers[FULL_BUF][i * n], &n_iterations, &d_data_buffer[i * k_]);
        DTL_LOG_DEBUG("decode: n_it={}", n_iterations);
    }

}


const std::vector<unsigned char>& tb_decoder::data() const { return d_data_buffer; }

} // namespace dtl
} // namespace gr