/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_IMPLH

#include <gnuradio/dtl/ofdm_adaptive_tx_control_bb.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>

namespace gr {
namespace dtl {

/*!
 * \brief Tag the input stream according to the feedback received.
 * \ingroup dtl
 *
 */
class ofdm_adaptive_tx_control_bb_impl : public ofdm_adaptive_tx_control_bb
{
public:
    ofdm_adaptive_tx_control_bb_impl(const std::string& len_tag_key, size_t packet_len);

    void process_feedback(pmt::pmt_t feedback);

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
    bool start() override;

private:
    constellation_type_t d_constellation;
    unsigned char d_fec_scheme;
    uint64_t d_tag_offset;
    size_t d_packet_len;
    pmt::pmt_t d_packet_len_tag;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_IMPL_H */