/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_FEC_UTILS_H
#define INCLUDED_DTL_FEC_UTILS_H

#include <cassert>
#include <gnuradio/dtl/fec.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

#include <gnuradio/tags.h>

namespace gr {
namespace dtl {


struct fec_info_t {

    typedef std::shared_ptr<fec_info_t> sptr;

    fec_enc::sptr d_enc;
    fec_dec::sptr d_dec;
    int d_frame_len;
    int d_tb_offset;
    int d_tb_frame_idx;
    int d_tb_number;
    int d_tb_payload_len;

    fec_info_t() = default;

    fec_info_t(fec_enc::sptr enc,
            fec_dec::sptr dec,
            int frame_len,
            int tb_offset,
            int tb_frame_idx,
            int tb_number,
            int tb_payload_len);

    int get_n();

    int get_k();
};


fec_info_t::sptr make_fec_info(const std::vector<tag_t> tags, const std::vector<fec_enc::sptr> encoders, const std::vector<fec_dec::sptr> decoders);

int compute_tb_len(int cw_len, int frame_len);

} // namespace dtl
} // namespace gr

#endif // INCLUDED_DTL_FEC_UTILS_H