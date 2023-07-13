/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tb_decoder.h"

#include "fec_utils.h"
#include "logger.h"
#include <cstring>

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("tb_decoder");

tb_decoder::tb_decoder(int max_tb_len)
    : d_payload(0),
      d_tb_number(-1),
      d_buf_idx(0),
      d_fec_info(nullptr),
      d_processed(0)
{
    DTL_LOG_DEBUG("max_tb_len={}", max_tb_len);
    d_tb_buffers.resize(2);
    d_tb_buffers[RCV_BUF].reserve(2 * max_tb_len);
    d_tb_buffers[FULL_BUF].reserve(2 * max_tb_len);
    d_data_buffer.reserve(2 * max_tb_len);
    d_tb_buffers[RCV_BUF].clear();
}

bool tb_decoder::process_frame(const float* in,
                               int frame_len,
                               int frame_payload_len,
                               int bps,
                               fec_info_t::sptr fec_info)
{
    bool data_ready = false;

    if (!fec_info) {
        DTL_LOG_DEBUG("process_frame: fec info fail");
        return false;
    }

    DTL_LOG_DEBUG("process_frame: current_no={}, rcvd_no={}, rcvd_offset={}, "
                  "frame_len={}, frame_payload={}",
                  d_tb_number,
                  fec_info->d_tb_number,
                  fec_info->d_tb_offset,
                  frame_len,
                  frame_payload_len);

    // If frame is part of the current TB
    if (fec_info->d_tb_number == d_tb_number) {
        if (d_buf_idx + frame_payload_len > d_tb_buffers[RCV_BUF].capacity()) {
            throw runtime_error("rcv buffer does not have enough capacity!");
        }
        copy(in, in + frame_payload_len, back_inserter(d_tb_buffers[RCV_BUF]));
        d_buf_idx += frame_payload_len;
        int ncws = compute_tb_len(d_fec_info->get_n(), frame_len);

        if (d_tb_buffers[RCV_BUF].size() >= expected_tb_len(fec_info, ncws)) {
            decode(ncws);
            d_buf_idx = 0;
            d_tb_buffers[RCV_BUF].clear();
            data_ready = true;
        }
        // If frame is part of a new TB
    } else {

        // Small TB exclusively transported by the frame
        if (d_tb_buffers[RCV_BUF].size() == 0 && fec_info->d_tb_offset == frame_payload_len) {
            // Start new TB buffer
            d_fec_info = fec_info;
            d_tb_number = d_fec_info->d_tb_number;
            d_tb_payload_len = d_fec_info->d_tb_payload_len;
            d_buf_idx = 0;
            d_tb_buffers[RCV_BUF].clear();

            copy(in, in + fec_info->d_tb_offset, back_inserter(d_tb_buffers[RCV_BUF]));

            int tb_len = compute_tb_len(d_fec_info->get_n(), frame_len);
            decode(tb_len);
            d_tb_buffers[RCV_BUF].clear();
            data_ready = true;
        } else {

            // Fill current TB buffer and decode current TB
            if (fec_info->d_tb_offset > 0) {
                copy(
                    in, in + fec_info->d_tb_offset, back_inserter(d_tb_buffers[RCV_BUF]));
            }

            // Decode
            if (d_tb_buffers[RCV_BUF].size() > 0) {
                int tb_len = compute_tb_len(d_fec_info->get_n(), frame_len);
                decode(tb_len);
                data_ready = true;
            }

            // Start new TB buffer
            d_fec_info = fec_info;
            d_tb_number = d_fec_info->d_tb_number;
            d_tb_payload_len = d_fec_info->d_tb_payload_len;
            d_buf_idx = 0;
            d_tb_buffers[RCV_BUF].clear();

            // align offset to symbols
            int extra_bits = 0;
            if (fec_info->d_tb_offset % bps) {
                extra_bits = bps - fec_info->d_tb_offset % bps;
            }
            int new_tb_offset = 0;
            if (fec_info->d_tb_offset) {
                new_tb_offset = fec_info->d_tb_offset + extra_bits;
            }
            copy(in + new_tb_offset,
                 in + frame_payload_len + extra_bits,
                 back_inserter(d_tb_buffers[RCV_BUF]));
            d_buf_idx += frame_payload_len - fec_info->d_tb_offset;
            DTL_LOG_DEBUG("rcv_buf_size={}, append={}",
                          d_tb_buffers[RCV_BUF].size(),
                          frame_payload_len + extra_bits - new_tb_offset);
        }
    }

    d_processed += frame_payload_len;

    return data_ready;
}

int tb_decoder::decode(int tb_len)
{
    static const float SHORTENED_VALUE = -5;

    int payload_len = d_fec_info->d_tb_payload_len;
    int n = d_fec_info->get_n();
    int k = d_fec_info->get_k();
    int ncheck = n - k;
    int n_iterations = 0;

    DTL_LOG_VEC("decode tb", d_tb_buffers[RCV_BUF]);
    DTL_LOG_DEBUG("decode: ncws={}, n_it={}, sz={}, tb_payload_len={}, n={}",
                  tb_len,
                  n_iterations,
                  d_tb_buffers[FULL_BUF].size(),
                  payload_len,
                  n);

    d_data_buffer.resize(d_fec_info->d_tb_payload_len);
    d_tb_buffers[FULL_BUF].clear();
    fill(
        &d_tb_buffers[FULL_BUF][0], &d_tb_buffers[FULL_BUF][tb_len * n], SHORTENED_VALUE);

    int rcv_idx = 0;
    for (int i = 0, d_idx = 0; i < tb_len; ++i) {
        int k_ = payload_len / (tb_len - i);
        if (payload_len % (tb_len - i)) {
            ++k_;
        }
        payload_len -= k_;

        // copy check bits
        copy(&d_tb_buffers[RCV_BUF][rcv_idx],
             &d_tb_buffers[RCV_BUF][rcv_idx + ncheck],
             d_tb_buffers[FULL_BUF].begin() + i * n);
        rcv_idx += ncheck;
        // copy systematic bits
        copy(&d_tb_buffers[RCV_BUF][rcv_idx],
             &d_tb_buffers[RCV_BUF][rcv_idx + k_],
             d_tb_buffers[FULL_BUF].begin() + i * n + ncheck);
        rcv_idx += k_;

        // NO SHORTENING
        // copy(&d_tb_buffers[RCV_BUF][i*n], &d_tb_buffers[RCV_BUF][(i+1)*n],
        // &d_tb_buffers[FULL_BUF][i * n]);

        d_fec_info->d_dec->decode(
            &d_tb_buffers[FULL_BUF][i * n], &n_iterations, &d_data_buffer[d_idx]);
        d_idx += k_;

        DTL_LOG_DEBUG("decode: n_it={}, k_={}", n_iterations, k_);
    }

    return tb_len * ncheck + d_fec_info->d_tb_payload_len;
}

pair<int, int> tb_decoder::buf_out(unsigned char* out)
{
    memcpy(out, &d_data_buffer[0], d_data_buffer.size());
    auto result = make_pair(static_cast<int>(d_data_buffer.size()), d_processed);
    d_processed = 0;
    return result;
}

std::size_t tb_decoder::expected_tb_len(fec_info_t::sptr fec_info, int ncws)
{
    int total_check = (fec_info->get_n() - fec_info->get_k()) * ncws;
    return total_check + fec_info->d_tb_payload_len;
}

} // namespace dtl
} // namespace gr