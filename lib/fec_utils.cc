
/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include "fec_utils.h"


namespace gr {
namespace dtl {


fec_info_t::fec_info_t(fec_enc::sptr enc,
        fec_dec::sptr dec,
        int frame_len,
        int tb_offset,
        int tb_frame_idx,
        int tb_number,
        int tb_payload_len)
    : d_enc(enc),
        d_dec(dec),
        d_frame_len(frame_len),
        d_tb_offset(tb_offset),
        d_tb_frame_idx(tb_frame_idx),
        d_tb_number(tb_number),
        d_tb_payload_len(tb_payload_len)
{
    if (d_enc != nullptr && d_dec != nullptr) {
        assert(d_enc->get_k() == d_enc->get_k());
        assert(d_enc->get_n() == d_enc->get_n());
    }
}

int fec_info_t::get_n()
{
    if (d_enc != nullptr) {
        return d_enc->get_n();
    } else if (d_dec != nullptr) {
        return d_dec->get_n();
    }
    return 0;
}

int fec_info_t::get_k()
{
    if (d_enc != nullptr) {
        return d_enc->get_k();
    } else if (d_dec != nullptr) {
        return d_dec->get_k();
    }
    return 0;
}

int fec_info_t::no_of_cws()
{
    int n = get_n();
    int ncws = 1;
    if (d_frame_len >= n) {
        ncws = 1 + d_frame_len / n;
    }
    return ncws;
}


fec_info_t::sptr make_fec_info(const std::vector<tag_t> tags, const std::vector<fec_enc::sptr> encoders, const std::vector<fec_dec::sptr> decoders)
{
    fec_info_t::sptr fec_info = std::make_shared<fec_info_t>();
    int tags_check = 0;
    for (auto& tag: tags) {
        if (tag.key == fec_key()) {
            tags_check |= 1;
            int fec_idx = pmt::to_long(tag.value);
            if (encoders.size() > 0 &&fec_idx < encoders.size()) {
                fec_info->d_enc = encoders[fec_idx];
            }
            if (decoders.size() > 0 &&fec_idx < decoders.size()) {
                fec_info->d_dec = decoders[fec_idx];
            }
        } else if (tag.key == fec_tb_key()) {
            tags_check |= 2;
            fec_info->d_tb_number = pmt::to_long(tag.value);
        } else if (tag.key == fec_offset_key()) {
            tags_check |= 4;
            fec_info->d_tb_offset = pmt::to_long(tag.value);
        // } else if (tag.key == fec_tb_index_key()) {
        //     tags_check &= 8;
        //     fec_info->d_tb_frame_idx = pmt::to_long(tag.value);
        } else if (tag.key == fec_tb_payload_key()) {
            fec_info->d_tb_payload_len = pmt::to_long(tag.value);
            tags_check |= 8;
        }
        if ((tags_check ^ 0xF) == 0) {
            return fec_info;
        }
    }
    return nullptr;
}

int compute_tb_len(int cw_len, int frame_len)
{
    //int frame_len = d_frame_capacity * bps;
    int ncws = 1;
    if (frame_len > cw_len) {
        ncws = 1 + frame_len / cw_len;
    }
    return ncws;
}

} // namespace dtl
} // namespace gr
