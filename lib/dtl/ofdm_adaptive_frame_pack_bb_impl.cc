/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/testbed/logger.h>
#include "ofdm_adaptive_frame_pack_bb_impl.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/testbed/monitor_msg.h>
#include <utility>

namespace gr {
namespace dtl {


static const pmt::pmt_t PAYLOAD_CRC_FAILED_KEY = pmt::mp("crc_fail_count");
static const pmt::pmt_t PAYLOAD_CRC_SUCCESS_KEY = pmt::mp("crc_ok_count");
static const pmt::pmt_t MONITOR_PORT = pmt::mp("monitor");

INIT_DTL_LOGGER("ofdm_adaptive_frame_pack_bb");

ofdm_adaptive_frame_pack_bb::sptr
ofdm_adaptive_frame_pack_bb::make(const std::string& tsb_tag_key,
                                  const std::string& packet_number_key,
                                  const std::string& frames_fname)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_pack_bb_impl>(
        tsb_tag_key, packet_number_key, frames_fname);
}


ofdm_adaptive_frame_pack_bb_impl::ofdm_adaptive_frame_pack_bb_impl(
    const std::string& tsb_tag_key,
    const std::string& packet_number_key,
    const std::string& frames_fname)
    : tagged_stream_block("ofdm_adaptive_frame_pack_bb",
                          io_signature::make(1, 1, sizeof(char)),
                          io_signature::make(1, 1, sizeof(char)),
                          tsb_tag_key),
      d_bits_per_symbol(1),
      d_len_tag_key(tsb_tag_key),
      d_repacker(d_bits_per_symbol, 8),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF),
      d_packet_number_key(pmt::mp(packet_number_key))
{
    message_port_register_out(MONITOR_PORT);
    if (!frames_fname.empty()) {
        d_frame_store = frame_file_store(frames_fname);
    }
    set_tag_propagation_policy(block::tag_propagation_policy_t::TPP_DONT);
}

ofdm_adaptive_frame_pack_bb_impl::~ofdm_adaptive_frame_pack_bb_impl() {}


void ofdm_adaptive_frame_pack_bb_impl::parse_length_tags(
    const std::vector<std::vector<tag_t>>& tags, gr_vector_int& n_input_items_reqd)
{
    tagged_stream_block::parse_length_tags(tags, n_input_items_reqd);

    constellation_type_t constellation_type = find_constellation_type(tags[0]);
    if (constellation_type_t::UNKNOWN != constellation_type) {
        d_bits_per_symbol = get_bits_per_symbol(constellation_type);
        set_relative_rate(d_bits_per_symbol, 8);
        d_repacker = repack(d_bits_per_symbol, 8);
    }
}


int ofdm_adaptive_frame_pack_bb_impl::work(int noutput_items,
                                           gr_vector_int& ninput_items,
                                           gr_vector_const_void_star& input_items,
                                           gr_vector_void_star& output_items)
{
    const unsigned char* in = (const unsigned char*)input_items[0];
    unsigned char* out = (unsigned char*)output_items[0];
    int n_written = 0;

    n_written =
        d_repacker.repack_lsb_first(in, ninput_items[0], out);

    if (ninput_items[0] * d_bits_per_symbol % 8) {
        --n_written;
    }

    bool crc_ok = d_crc.verify_crc(out, n_written);

    pmt::pmt_t msg = monitor_msg(
                    std::make_pair(PAYLOAD_CRC_SUCCESS_KEY, d_crc.get_success()),
                    std::make_pair(PAYLOAD_CRC_FAILED_KEY, d_crc.get_failed()),
                    std::make_pair("payload_fer", d_crc.get_fail_rate()));

    message_port_pub(MONITOR_PORT, msg);

    DTL_LOG_DEBUG("d_bits_per_symbol={}, n_written={}, "
                  "ninput_items={}, crc_ok={}",
                  d_bits_per_symbol,
                  n_written,
                  ninput_items[0],
                  crc_ok);

    // Propagate tags
    std::vector<tag_t> tags;
    this->get_tags_in_range(
        tags, 0, this->nitems_read(0), this->nitems_read(0) + ninput_items[0]);

    auto it = find_tag(tags, d_packet_number_key);
    if (it != tags.end()) {
        d_frame_store.store((n_written - d_crc.get_crc_len()) & 0xFFF,
                            pmt::to_long(it->value),
                            reinterpret_cast<char*>(out));
    }

    add_item_tag(0,
                 nitems_written(0),
                 pmt::string_to_symbol(d_len_tag_key),
                 pmt::from_long(n_written - d_crc.get_crc_len()));

    return n_written - d_crc.get_crc_len();
}

} /* namespace dtl */
} /* namespace gr */
