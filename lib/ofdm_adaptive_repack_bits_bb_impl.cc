/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_repack_bits_bb_impl.h"

#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {


INIT_DTL_LOGGER("ofdm_adaptive_repack_bits_bb");


ofdm_adaptive_repack_bits_bb::sptr ofdm_adaptive_repack_bits_bb::make(
    const std::string& tsb_tag_key, bool unpack, endianness_t endianness)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_repack_bits_bb_impl>(
        tsb_tag_key, unpack, endianness);
}


ofdm_adaptive_repack_bits_bb_impl::ofdm_adaptive_repack_bits_bb_impl(
    const std::string& len_tag_key, bool bytes_to_symbols, endianness_t endianness)
    : tagged_stream_block("ofdm_adaptive_repack_bits_bb",
                          io_signature::make(1, 1, sizeof(char)),
                          io_signature::make(1, 1, sizeof(char)),
                          len_tag_key),
      d_bits_per_symbol(1),
      d_endianness(endianness),
      d_unpack(bytes_to_symbols),
      d_len_tag_key(len_tag_key)
{
}

ofdm_adaptive_repack_bits_bb_impl::~ofdm_adaptive_repack_bits_bb_impl() {}


void ofdm_adaptive_repack_bits_bb_impl::parse_length_tags(
    const std::vector<std::vector<tag_t>>& tags, gr_vector_int& n_input_items_reqd)
{
    tagged_stream_block::parse_length_tags(tags, n_input_items_reqd);

    constellation_type_t constellation_type = find_constellation_type(tags[0]);
    if (constellation_type_t::UNKNOWN != constellation_type) {
        d_bits_per_symbol = get_bits_per_symbol(constellation_type);
        if (d_unpack) {
            set_relative_rate(8, d_bits_per_symbol);
        } else {
            set_relative_rate(d_bits_per_symbol, 8);
        }
    }
}


int ofdm_adaptive_repack_bits_bb_impl::work(int noutput_items,
                                            gr_vector_int& ninput_items,
                                            gr_vector_const_void_star& input_items,
                                            gr_vector_void_star& output_items)
{
    const unsigned char* in = (const unsigned char*)input_items[0];
    unsigned char* out = (unsigned char*)output_items[0];
    int n_written = 0;

    switch (d_endianness) {
    case GR_LSB_FIRST:
        n_written = repacker.repack_lsb_first(
            in, ninput_items[0], out, d_bits_per_symbol, d_unpack);
        break;


    case GR_MSB_FIRST:
        n_written = repacker.repack_msb_first(
            in, ninput_items[0], out, d_bits_per_symbol, d_unpack);
        break;

    default:
        throw std::runtime_error(
            "ofdm_adaptive_repack_bits_bb: unrecognized endianness value.");
    }

    DTL_LOG_DEBUG("d_bits_per_symbol: {}, d_unpack: {}, n_written: {}, "
                  "noutput_items: {}",
                  d_bits_per_symbol,
                  d_unpack,
                  n_written,
                  noutput_items);

    // Propagate tags
    std::vector<tag_t> tags;
    this->get_tags_in_range(
        tags, 0, this->nitems_read(0), this->nitems_read(0) + ninput_items[0]);
    auto it = find_constellation_tag(tags);
    if (it != tags.end()) {
        add_item_tag(0, nitems_written(0), it->key, it->value);
        remove_item_tag(0, *it);
    }

    add_item_tag(0,
                 nitems_written(0),
                 pmt::string_to_symbol(d_len_tag_key),
                 pmt::from_long(n_written));

    return n_written;
}

} /* namespace dtl */
} /* namespace gr */
