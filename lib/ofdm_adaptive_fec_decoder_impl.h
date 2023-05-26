/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_IMPL_H

#include <gnuradio/dtl/fec.h>
#include <gnuradio/dtl/ofdm_adaptive_fec_decoder.h>
#include "tb_decoder.h"

namespace gr {
namespace dtl {

class ofdm_adaptive_fec_decoder_impl : public ofdm_adaptive_fec_decoder
{
private:
    pmt::pmt_t d_len_key;
    std::vector<fec_dec::sptr> d_decoders;
    int d_frame_capacity;
    tb_decoder::sptr d_tb_dec;
    bool d_data_ready;
    bool d_processed_input;

public:
    ofdm_adaptive_fec_decoder_impl(const std::vector<fec_dec::sptr> decoders, int frame_capacity, int max_bps, const std::string& len_key);
    ~ofdm_adaptive_fec_decoder_impl();

    // Where all the action really happens
    int general_work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_IMPL_H */
