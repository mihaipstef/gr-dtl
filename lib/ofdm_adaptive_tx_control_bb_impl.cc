/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include "logger.h"
#include "ofdm_adaptive_tx_control_bb_impl.h"

namespace gr {
namespace dtl {


using namespace gr;


INIT_DTL_LOGGER("ofdm_adaptive_tx_control_bb")


ofdm_adaptive_tx_control_bb::sptr
ofdm_adaptive_tx_control_bb::make(const std::string& len_tag_key, size_t packet_len)
{
    return std::make_shared<ofdm_adaptive_tx_control_bb_impl>(len_tag_key, packet_len);
}


ofdm_adaptive_tx_control_bb_impl::ofdm_adaptive_tx_control_bb_impl(
    const std::string& len_tag_key, size_t packet_len)
    : sync_block("ofdm_adaptive_tx_control_bb",
                 io_signature::make(1, 1, sizeof(char)),
                 io_signature::make(1, 1, sizeof(char))),
      d_constellation(constellation_type_t::BPSK),
      d_fec_scheme(0),
      d_tag_offset(0),
      d_packet_len(packet_len),
      d_packet_len_tag(pmt::mp(len_tag_key))
{
    this->message_port_register_in(pmt::mp("feedback"));
    this->set_msg_handler(pmt::mp("feedback"),
                          [this](pmt::pmt_t msg) { this->process_feedback(msg); });
}


void ofdm_adaptive_tx_control_bb_impl::process_feedback(pmt::pmt_t feedback)
{
    if (pmt::is_dict(feedback)) {
        if (pmt::dict_has_key(feedback, feedback_constellation_key())) {
            d_constellation =
                static_cast<constellation_type_t>(pmt::to_long(pmt::dict_ref(
                    feedback,
                    feedback_constellation_key(),
                    pmt::from_long(static_cast<int>(constellation_type_t::BPSK)))));
        }
        if (pmt::dict_has_key(feedback, feedback_fec_key())) {
            d_fec_scheme = pmt::to_long(
                pmt::dict_ref(feedback, feedback_fec_key(), pmt::from_long(0)));
        }
    }
}


int ofdm_adaptive_tx_control_bb_impl::work(int noutput_items,
                                           gr_vector_const_void_star& input_items,
                                           gr_vector_void_star& output_items)
{
    gr::thread::scoped_lock guard(d_setlock);

    auto in = static_cast<const unsigned char*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    memcpy(out, in, noutput_items);

    while (d_tag_offset < nitems_written(0) + noutput_items) {
        add_item_tag(0, d_tag_offset, d_packet_len_tag, pmt::from_long(d_packet_len));
        add_item_tag(0,
                     d_tag_offset,
                     get_constellation_tag_key(),
                     pmt::from_long(static_cast<int>(d_constellation)));
        d_tag_offset += d_packet_len;
    }
    return noutput_items;
}


bool ofdm_adaptive_tx_control_bb_impl::start()
{
    d_tag_offset = 0;
    return true;
}

} /* namespace dtl */
} /* namespace gr */
