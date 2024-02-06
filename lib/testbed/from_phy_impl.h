/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_TESTBED_FROM_PHY_IMPL_H
#define INCLUDED_TESTBED_FROM_PHY_IMPL_H

#include <gnuradio/testbed/phy_converge.h>


namespace gr {
namespace dtl {

class from_phy_impl : public from_phy
{
private:
    size_t d_tail_packet_len;
    size_t d_expected_len;
    pmt::pmt_t d_len_key;
    transported_protocol_t d_protocol;
    packet_validator::sptr d_validator;
    size_t d_offset_out;

    bool is_valid(const uint8_t* buf, size_t buf_len, size_t& packet_len);
    size_t copy_pdu(uint8_t* out, const uint8_t* buf, size_t len);


protected:
    void update_length_tags(int n_produced, int n_ports) override;
    int calculate_output_stream_length(const gr_vector_int& ninput_items) override;


public:
    from_phy_impl(transported_protocol_t protocol, packet_validator::sptr validator, const std::string& len_key);
    ~from_phy_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_TESTBED_FROM_PHY_IMPL_H */
