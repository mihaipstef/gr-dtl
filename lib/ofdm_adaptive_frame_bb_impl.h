/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_frame_bb.h>

namespace gr {
namespace dtl {

/*!
 * \brief Tag the input stream according to the feedback received.
 * \ingroup dtl
 *
 */
class ofdm_adaptive_frame_bb_impl : public ofdm_adaptive_frame_bb
{
public:
    ofdm_adaptive_frame_bb_impl(const std::string& len_tag_key,
                                     size_t frame_len,
                                     size_t n_payload_carriers);

    void process_feedback(pmt::pmt_t feedback);

    int general_work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
    bool start() override;
    void set_constellation(constellation_type_t constellation) override;

protected:
    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

private:

    size_t frame_length_bits(size_t frame_len, size_t n_payload_carriers, size_t bits_per_symbol);
    size_t frame_length();


    constellation_type_t d_constellation;
    unsigned char d_fec_scheme;
    uint64_t d_tag_offset;
    size_t d_frame_len;
    pmt::pmt_t d_packet_len_tag;
    size_t d_payload_carriers;
    size_t d_bytes;
    unsigned char d_bps;
    bool d_waiting_full_frame;
    bool d_waiting_for_input;
    bool d_stop_no_input;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H */