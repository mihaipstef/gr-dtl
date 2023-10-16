/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_IP_PACKET_IMPL_H
#define INCLUDED_DTL_IP_PACKET_IMPL_H

#include <gnuradio/dtl/packet_defragmentation.h>


namespace gr {
namespace dtl {

class packet_defragmentation_impl : public packet_defragmentation
{
private:
    size_t d_current_packet_len;
    size_t d_expected_len;
    pmt::pmt_t d_len_key;
    packet_validator::sptr d_validator;

public:
    packet_defragmentation_impl(packet_validator::sptr validator, const std::string& len_key);
    ~packet_defragmentation_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_IP_PACKET_IMPL_H */
