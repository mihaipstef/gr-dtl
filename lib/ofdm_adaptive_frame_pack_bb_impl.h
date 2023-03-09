/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_PACK_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_PACK_BB_IMPL_H

#include "crc_util.h"
#include <gnuradio/blocks/repack_bits_bb.h>
#include <gnuradio/dtl/ofdm_adaptive_frame_pack_bb.h>
#include "repack.h"

namespace gr {
namespace dtl {

class ofdm_adaptive_frame_pack_bb_impl : public ofdm_adaptive_frame_pack_bb
{
private:
    unsigned char d_bits_per_symbol;
    std::string d_len_tag_key;
    repack repacker;
    crc_util d_crc;


protected:
    void parse_length_tags(const std::vector<std::vector<tag_t>>& tags,
                                            gr_vector_int& n_input_items_reqd) override;

public:
    ofdm_adaptive_frame_pack_bb_impl(const std::string& tsb_tag_key);
    ~ofdm_adaptive_frame_pack_bb_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_PACK_BB_IMPL_H */
