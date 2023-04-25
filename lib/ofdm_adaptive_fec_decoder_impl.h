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

namespace gr {
namespace dtl {

class ofdm_adaptive_fec_decoder_impl : public ofdm_adaptive_fec_decoder
{
private:

    void fill_shortened(int tb_len, int n_cw, int k, int n);
    int decode(unsigned char* out, int tb_payload_len, int n_cw, int fec_idx);

    std::vector<fec_dec::sptr> d_decoders;
    std::vector<std::vector<float>> d_buffers;
    int d_frame_len;
    int d_tb_remaining;
    int d_tb_len;
    int d_last_current_tb_id;
    int d_last_previous_tb_id;
    int d_last_tb_index;
    int d_tb_buffer_idx;
    int d_current_tb_id;
    int d_previous_tb_id;
    int d_tb_offset;
    int d_tb_index;
    int d_fec_idx;
    int d_last_fec_idx;
    int d_frame_payload_len;
    int d_tb_payload_len;
    bool d_tb_compromised;
    bool d_ready_to_decode;
    int d_frame_len_syms;
    int d_n_cw;

protected:

public:
    ofdm_adaptive_fec_decoder_impl(const std::string& len_key);
    ~ofdm_adaptive_fec_decoder_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_DECODER_IMPL_H */
