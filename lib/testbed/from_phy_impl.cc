/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "from_phy_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/testbed/logger.h>
#include <tuple>


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("from_phy")

from_phy::sptr from_phy::make(transported_protocol_t protocol, packet_validator::sptr validator, const std::string& len_key)
{
    return gnuradio::make_block_sptr<from_phy_impl>(protocol, validator, len_key);
}


/*
 * The private constructor
 */
from_phy_impl::from_phy_impl(transported_protocol_t protocol, packet_validator::sptr validator, const std::string& len_key)
    : gr::tagged_stream_block("from_phy",
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              len_key),
      d_tail_packet_len(0),
      d_expected_len(0),
      d_len_key(pmt::mp(len_key)),
      d_protocol(protocol),
      d_validator(validator),
      d_offset_out(0),
      d_tag_offset(0)
{
}


from_phy_impl::~from_phy_impl() {}


size_t from_phy_impl::copy_pdu(uint8_t* out, const uint8_t* buf, size_t len)
{
    if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
        memcpy(out, buf, 12);
        memcpy(out+12, buf+14, len-12);
        return len - 2;
    } else {
        memcpy(out, buf, len);
        return len;
    }

}


bool from_phy_impl::is_valid(const uint8_t* buf, size_t buf_len, size_t& packet_len)
{
    auto result = d_validator->valid(buf, buf_len);
    packet_len = std::get<1>(result);
    return std::get<0>(result);
}


int from_phy_impl::calculate_output_stream_length(const gr_vector_int& ninput_items)
{
    if (d_expected_len) {
        return d_expected_len;
    }
    return tagged_stream_block::calculate_output_stream_length(ninput_items);
}


int from_phy_impl::work(int noutput_items,
                         gr_vector_int& ninput_items,
                         gr_vector_const_void_star& input_items,
                         gr_vector_void_star& output_items)
{
    auto in = static_cast<const uint8_t*>(input_items[0]);
    auto out = static_cast<uint8_t*>(output_items[0]);
    size_t input_buf_len = ninput_items[0];
    size_t offset_in = 0;

    DTL_LOG_DEBUG("work start: ninput={}, noutput={}",  ninput_items[0], noutput_items);

    while (offset_in < input_buf_len) {
        size_t packet_len = 0;
        bool valid_packet = is_valid(&in[offset_in], input_buf_len - offset_in, packet_len);

        DTL_LOG_DEBUG("is_valid: result={}, d_tail_packet_len={}, packet_len={}, expected_len={}", valid_packet, d_tail_packet_len, packet_len, d_expected_len);

        // If buffer starts with expected frame/packet header
        if (valid_packet) {

            // If unfinished frame/packet
            if (d_tail_packet_len) {
                // leave it fot the upper layers to handle as it is
                add_item_tag(0, nitems_written(0) + d_tag_offset, d_len_key, pmt::from_long(d_tail_packet_len));
                DTL_LOG_DEBUG("add tag offset={}, value={}, tag_offset={}", nitems_written(0), d_tail_packet_len, d_tag_offset);
                d_tag_offset += d_tail_packet_len;
                d_tail_packet_len = 0;
            }

            // Start with new frame/packet
            d_tail_packet_len = 0;
            if (d_protocol == transported_protocol_t::MODIFIED_ETHER) {
                d_expected_len = packet_len - 2;
            } else {
                d_expected_len = packet_len;
            }
            // If frame/packet is all available in the buffer
            if (offset_in + packet_len <= input_buf_len) {
                // copy to output and mark it ready
                size_t produced = copy_pdu(&out[d_offset_out], &in[offset_in], packet_len);
                add_item_tag(0, nitems_written(0) + d_tag_offset, d_len_key, pmt::from_long(produced));
                DTL_LOG_DEBUG("add tag offset={}, value={}, tag_offset={}", nitems_written(0) + d_offset_out, produced, d_tag_offset);
                offset_in += packet_len;
                d_offset_out += produced;
                d_tag_offset += produced;
            // If frame/packet is not available in the buffer...
            } else {
                // ...start jumbo mode processing.
                size_t produced = copy_pdu(&out[d_offset_out], &in[offset_in], input_buf_len - offset_in);
                d_tail_packet_len = produced;
                d_offset_out += produced;
                assert(d_tail_packet_len < d_expected_len);
                offset_in = input_buf_len; // Consumed everything
            }
        // If buffer doesn't start with expected header
        } else {
            // If jumbo mode...
            if (d_tail_packet_len) {
                // ...handle as jumbo frame/packet.
                int to_consume = std::min(d_expected_len - d_tail_packet_len, input_buf_len - offset_in);
                memcpy(&out[d_offset_out], &in[offset_in], to_consume);
                offset_in += to_consume;
                d_offset_out += to_consume;
                d_tail_packet_len += to_consume;
                if (d_tail_packet_len == d_expected_len) {
                    //done
                    add_item_tag(0, nitems_written(0) + d_tag_offset, d_len_key, pmt::from_long(d_expected_len));
                    DTL_LOG_DEBUG("add tag offset={}, value={}, tag_offset={}", nitems_written(0), d_expected_len, d_tag_offset);
                    d_tag_offset += d_expected_len;
                    d_tail_packet_len = 0;
                    d_expected_len = 0;
                }
            //otherwise...
            } else {
                // ...try output everything in a PDU for upper layer.
                size_t to_consume = input_buf_len - offset_in;
                if (packet_len > 14) {
                    to_consume = std::min(to_consume, packet_len);
                }
                memcpy(&out[d_offset_out], &in[offset_in], to_consume);
                add_item_tag(0, nitems_written(0) + d_tag_offset, d_len_key, pmt::from_long(to_consume));
                DTL_LOG_DEBUG("add tag offset={}, value={}, tag_offset={}", nitems_written(0) + d_offset_out, to_consume, d_tag_offset);
                d_tag_offset += to_consume;
                d_offset_out += to_consume;
                offset_in += to_consume;
            }
        }
        DTL_LOG_DEBUG("step: offset_in={}, offset_out={}, d_tail_packet_len={}, expected_len={}",
            offset_in, d_offset_out, d_tail_packet_len, d_expected_len);
    }

    if (d_tail_packet_len) {
        return 0;
    }

    DTL_LOG_BUFFER("Packet out", out, d_offset_out);
    int produced = d_offset_out;
    d_offset_out = 0;
    d_tag_offset = 0;
    return produced;
}


void from_phy_impl::update_length_tags(int n_produced, int n_ports)
{
    // DO NOTHING
}



} /* namespace dtl */
} /* namespace gr */
