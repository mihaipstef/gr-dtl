/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_BVB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_BVB_IMPL_H

#include "crc_util.h"
#include <chrono>
#include <gnuradio/dtl/fec.h>
#include <gnuradio/dtl/ofdm_adaptive_fec_frame_bvb.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include "tb_encoder.h"


namespace gr {
namespace dtl {


class ofdm_adaptive_fec_frame_bvb_impl : public ofdm_adaptive_fec_frame_bvb
{
private:
    enum class Action { PROCESS_INPUT, OUTPUT_BUFFER, FINALIZE_FRAME };

    void add_frame_tags(int frame_payload);
    void padded_frame_out(int frame_payload);
    int tb_offset_to_bytes();
    int current_frame_available_bytes();
    int align_bytes_to_syms(int nbytes);

    std::vector<fec_enc::sptr> d_encoders;
    int d_frame_capacity;
    int d_tb_len; // [bits] Transport block length (frame_len [bits] < d_tb_len <= 2 *
                  // frame_len [bits])
    unsigned long d_tb_count;
    unsigned long d_cw_count;

    unsigned char d_header_fec_idx;
    constellation_type_t d_header_cnst;
    fec_enc::sptr d_current_enc;
    unsigned char d_current_fec_idx;
    constellation_type_t d_current_cnst;
    int d_current_bps;
    tb_encoder::sptr d_tb_enc;
    int d_current_frame_len;
    int d_current_frame_offset;
    int d_current_frame_payload;
    pmt::pmt_t d_len_key;
    uint64_t d_tag_offset;
    Action d_action;
    int d_used_frames_count;
    int d_frame_padding_syms;
    int d_frame_used_capacity;
    int d_consecutive_empty_frames;
    std::vector<unsigned char> d_tb_payload;
    std::vector<unsigned char> d_crc_buffer;
    crc_util d_crc;
    std::chrono::time_point<std::chrono::steady_clock> d_start_time;
    unsigned long d_total_frames;
    std::chrono::duration<double> d_frame_duration;
    int d_max_empty_frames;
    unsigned char d_feedback_fec_idx;
    constellation_type_t d_feedback_cnst;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> d_expected_time;

public:
    ofdm_adaptive_fec_frame_bvb_impl(const std::vector<fec_enc::sptr>& encoders,
                                     int frame_capacity,
                                     double frame_rate,
                                     int max_bps,
                                     int max_empty_frames,
                                     const std::string& len_key);
    ~ofdm_adaptive_fec_frame_bvb_impl();

    void process_feedback(pmt::pmt_t feedback);

    void process_feedback_header(pmt::pmt_t header_data);

    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

    // Where all the action really happens
    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items);

    bool start() override;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_BVB_IMPL_H */
