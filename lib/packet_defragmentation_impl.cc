/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "packet_defragmentation_impl.h"
#include "logger.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("packet_defragmentation");

packet_defragmentation::sptr packet_defragmentation::make(packet_validator::sptr validator, const std::string& len_key)
{
    return gnuradio::make_block_sptr<packet_defragmentation_impl>(validator, len_key);
}

/*
 * The private constructor
 */
packet_defragmentation_impl::packet_defragmentation_impl(packet_validator::sptr validator, const std::string& len_key)
    : gr::tagged_stream_block("packet_defragmentation",
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              gr::io_signature::make(1, 1, sizeof(uint8_t)),
                              len_key),
      d_current_packet_len(0),
      d_expected_len(0),
      d_len_key(pmt::mp(len_key)),
      d_validator(validator)
{
}


packet_defragmentation_impl::~packet_defragmentation_impl() {}



int packet_defragmentation_impl::work(int noutput_items,
                         gr_vector_int& ninput_items,
                         gr_vector_const_void_star& input_items,
                         gr_vector_void_star& output_items)
{
    auto in = static_cast<const uint8_t*>(input_items[0]);
    auto out = static_cast<uint8_t*>(output_items[0]);

    int packet_len = d_validator->valid(&in[0], ninput_items[0]);

    DTL_LOG_DEBUG("ninput={}, noutput={}, d_packet_len={}, expected_len={}",
                  ninput_items[0],
                  noutput_items,
                  d_current_packet_len,
                  d_expected_len);

    DTL_LOG_BUFFER("ether: ", &in[0], 14);

    if (packet_len >= 0) {
        DTL_LOG_DEBUG("valid packet! {}", packet_len);
        if (d_current_packet_len) {
            memcpy(&out[d_current_packet_len], in, ninput_items[0]);
            size_t produced = d_current_packet_len;
            d_current_packet_len = ninput_items[0];
            DTL_LOG_DEBUG("produced={}", produced);

            return produced;
        }
        d_expected_len = packet_len;
    }

    memcpy(&out[d_current_packet_len], in, ninput_items[0]);
    d_current_packet_len += ninput_items[0];
    if (d_current_packet_len < d_expected_len) {
        return 0;
    }

    size_t produced = d_expected_len;

    DTL_LOG_DEBUG("produced={}", produced);

    d_current_packet_len = 0;
    d_expected_len = 0;
    return produced;
}

} /* namespace dtl */
} /* namespace gr */
