/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "to_phy_impl.h"
#include <arpa/inet.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/testbed/logger.h>
#include <gnuradio/testbed/repack.h>

#include <tuple>


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("to_phy")


const pmt::pmt_t pdu_in()
{
    static const pmt::pmt_t val = pmt::mp("pdus");
    return val;
}


to_phy::sptr to_phy::make(transported_protocol_t protocol, const std::string& len_key)
{
    return gnuradio::make_block_sptr<to_phy_impl>(protocol, len_key);
}


/*
 * The private constructor
 */
to_phy_impl::to_phy_impl(transported_protocol_t protocol, const std::string& len_key)
    : gr::tagged_stream_block("to_phy",
                              gr::io_signature::make(0, 0, 0),
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              ""),
      d_protocol(protocol),
      d_len_key(pmt::mp(len_key)),
      d_pdu_len(0),
      d_in_consumed(0),
      d_out_used(0)
{
    message_port_register_in(pdu_in());
}


to_phy_impl::~to_phy_impl() {}


int to_phy_impl::next_pdu(const gr_vector_int& ninput_items)
{
    pmt::pmt_t msg(delete_head_nowait(pdu_in()));
    if (msg.get() == NULL) {
        return 0;
    }
    d_pdu = pmt::cdr(msg);
    int len = pmt::blob_length(d_pdu);
    DTL_LOG_DEBUG("PDU in: len={}", len);
    return len;
}


void to_phy_impl::forecast(int noutput_items,
                                           gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = 0;
}


bool to_phy_impl::is_output_enough(int noutput_items, int pdu_len)
{
    if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
        return noutput_items >= 14;
    }
    return noutput_items >= 12;
}


int to_phy_impl::work(int noutput_items,
                         gr_vector_int& ninput_items,
                         gr_vector_const_void_star& input_items,
                         gr_vector_void_star& output_items)
{
    DTL_LOG_DEBUG("work_start: noutput={}, consumed={}, d_pdu_len={}, queue={}", noutput_items, d_in_consumed, d_pdu_len, nmsgs(pdu_in()));

    auto out = static_cast<uint8_t*>(output_items[0]);
    //repack r(8, 8/d_bpb);
    int outed = 0;

    while (outed < noutput_items && nmsgs(pdu_in())) {

        if (d_pdu_len == 0) {
            d_pdu_len = next_pdu(ninput_items);
            d_in_consumed = 0;
            d_out_used = 0;
            if (d_pdu_len == 0) {
                return 0;
            }
        }

        if (!is_output_enough(noutput_items, d_pdu_len)) {
            break;
        }

        size_t len = 0;
        const uint8_t* in = (const uint8_t*)uniform_vector_elements(d_pdu, len);

        if (d_in_consumed == 0) {
            size_t pdu_len = len;
            size_t tag_offset = outed;
            //outed += r.repack_lsb_first(in, 12, out + outed);
            memcpy(out + outed, in, 12);
            outed += 12;
            if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
                pdu_len += 2;
                uint8_t len_buf[2] = { (pdu_len >> 8) & 0xff, pdu_len & 0xff };
                //outed += r.repack_lsb_first(len_buf, 2, out + outed);
                memcpy(out + outed, len_buf, 22);
                outed += 2;
            }
            d_in_consumed = 12;
            //add_item_tag(0, nitems_written(0) + tag_offset, d_len_key, pmt::from_long(pdu_len*d_bpb));
            add_item_tag(0, nitems_written(0) + tag_offset, d_len_key, pmt::from_long(pdu_len));
        }
        // size_t to_consume = std::min(noutput_items - outed, (int)(d_pdu_len - d_in_consumed)*d_bpb)/d_bpb;
        // outed += r.repack_lsb_first(in+d_in_consumed, to_consume, out + outed);
        size_t to_consume = std::min(noutput_items - outed, (int)(d_pdu_len - d_in_consumed));
        memcpy(out+outed, in+d_in_consumed, to_consume);
        outed += to_consume;
        d_in_consumed += to_consume;
        d_out_used += outed;
        DTL_LOG_DEBUG("end: used={}, consumed={}, d_pdu_len={}, to_consume={}, outed={}", d_out_used, d_in_consumed, d_pdu_len, to_consume, outed);

        if (d_in_consumed == d_pdu_len) {
            d_pdu_len = 0;
        }
    }
    return outed;
}


} /* namespace dtl */
} /* namespace gr */
