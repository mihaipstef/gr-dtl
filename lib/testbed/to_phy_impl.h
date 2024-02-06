/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_TESTBED_TO_PHY_IMPL_H
#define INCLUDED_TESTBED_TO_PHY_IMPL_H

#include <gnuradio/testbed/phy_converge.h>


namespace gr {
namespace dtl {

class to_phy_impl : public to_phy
{
private:
    transported_protocol_t d_protocol;
    pmt::pmt_t d_len_key;
    size_t d_pdu_len;
    pmt::pmt_t d_pdu;

protected:
    int calculate_output_stream_length(const gr_vector_int& ninput_items) override;

public:
    to_phy_impl(transported_protocol_t protocol, const std::string& len_key);
    ~to_phy_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_TESTBED_TO_PHY_IMPL_H */
