/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_fec_pack_bb.h>
#include "repack.h"

namespace gr {
namespace dtl {

class ofdm_adaptive_fec_pack_bb_impl : public ofdm_adaptive_fec_pack_bb
{
private:
    pmt::pmt_t d_len_key;
    repack packer;
public:
    ofdm_adaptive_fec_pack_bb_impl(const std::string& len_key);
    ~ofdm_adaptive_fec_pack_bb_impl();

    void forecast(int noutput_items,
                                   gr_vector_int& ninput_items_required) override;

    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_PACK_BB_IMPL_H */
