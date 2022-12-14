/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_repack_bits_bb_impl.h"
#include "logger.h"

#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {


INIT_DTL_LOGGER(__FILE__);


ofdm_adaptive_repack_bits_bb::sptr
ofdm_adaptive_repack_bits_bb::make(const std::string& tsb_tag_key,
                                   bool unpack,
                                   endianness_t endianness)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_repack_bits_bb_impl>(
        tsb_tag_key, unpack, endianness);
}


ofdm_adaptive_repack_bits_bb_impl::ofdm_adaptive_repack_bits_bb_impl(
    const std::string& len_tag_key,
    bool bytes_to_symbols,
    endianness_t endianness)
    : tagged_stream_block("ofdm_adaptive_repack_bits_bb",
                          io_signature::make(1, 1, sizeof(char)),
                          io_signature::make(1, 1, sizeof(char)),
                          len_tag_key),
      d_bits_per_in_byte(8),
      d_bits_per_out_byte(4),
      d_endianness(endianness),
      d_unpack(bytes_to_symbols),
      d_len_tag_key(len_tag_key)
{
    set_relative_rate(d_bits_per_in_byte, d_bits_per_out_byte);
}

ofdm_adaptive_repack_bits_bb_impl::~ofdm_adaptive_repack_bits_bb_impl() {}

int ofdm_adaptive_repack_bits_bb_impl::calculate_output_stream_length(
    const gr_vector_int& ninput_items)
{
    int n_out_bytes =
        (ninput_items[0] * d_bits_per_in_byte) / d_bits_per_out_byte;
    if ((ninput_items[0] * d_bits_per_in_byte) % d_bits_per_out_byte) {
        n_out_bytes += static_cast<int>(!d_unpack);
    }
    return n_out_bytes;
}


void ofdm_adaptive_repack_bits_bb_impl::parse_length_tags(const std::vector<std::vector<tag_t>>& tags,
                                            gr_vector_int& n_input_items_reqd)
{
    tagged_stream_block::parse_length_tags(tags, n_input_items_reqd);

    constellation_type_t constellation_type = get_constellation_type(
        tags[0]
    );
    if (constellation_type_t::UNKNOWN != constellation_type) {
        unsigned char bits_per_constellation = compute_no_of_bits_per_symbol(
            constellation_type
        );
        if (d_unpack) {
            d_bits_per_in_byte = 8;
            d_bits_per_out_byte = bits_per_constellation;
        }
        else {
            d_bits_per_in_byte = bits_per_constellation;
            d_bits_per_out_byte = 8;
        }
        set_relative_rate(d_bits_per_in_byte, d_bits_per_out_byte);
    }
}



int ofdm_adaptive_repack_bits_bb_impl::work(int noutput_items,
                                            gr_vector_int& ninput_items,
                                            gr_vector_const_void_star& input_items,
                                            gr_vector_void_star& output_items)
{
    const unsigned char* in = (const unsigned char*)input_items[0];
    unsigned char* out = (unsigned char*)output_items[0];
    int bytes_to_read = ninput_items[0];
    int bytes_to_write = bytes_to_read * d_bits_per_in_byte / d_bits_per_out_byte;

    if (((bytes_to_read * d_bits_per_in_byte) % d_bits_per_out_byte) != 0) {
        bytes_to_write += static_cast<int>(d_unpack);
    }

    DTL_LOG_BYTES("in:", in, bytes_to_read)

    int n_read = 0;
    int n_written = 0;

    //TODO: declare this local
    unsigned char out_index = 0;
    unsigned in_index = 0;

    switch (d_endianness) {
    case GR_LSB_FIRST:
        while (n_written < bytes_to_write && n_read < ninput_items[0]) {
            if (out_index == 0) {
                out[n_written] = 0;
            }
            out[n_written] |= ((in[n_read] >> in_index) & 0x01) << out_index;

            in_index = (in_index + 1) % d_bits_per_in_byte;
            out_index = (out_index + 1) % d_bits_per_out_byte;
            if (in_index == 0) {
                n_read++;
                in_index = 0;
            }
            if (out_index == 0) {
                n_written++;
                out_index = 0;
            }
        }

        if (out_index) {
            n_written++;
            out_index = 0;
        }

        break;


    case GR_MSB_FIRST:
        while (n_written < bytes_to_write && n_read < ninput_items[0]) {
            if (out_index == 0) {
                out[n_written] = 0;
            }
            out[n_written] |=
                ((in[n_read] >> (d_bits_per_in_byte - 1 - in_index)) & 0x01)
                << (d_bits_per_out_byte - 1 - out_index);

            in_index = (in_index + 1) % d_bits_per_in_byte;
            out_index = (out_index + 1) % d_bits_per_out_byte;
            if (in_index == 0) {
                n_read++;
                in_index = 0;
            }
            if (out_index == 0) {
                n_written++;
                out_index = 0;
            }
        }

        if (out_index) {
            n_written++;
            out_index = 0;
        }

        break;

    default:
        throw std::runtime_error(
            "ofdm_adaptive_repack_bits_bb: unrecognized endianness value.");
    }

    // Propagate tags
    std::vector<tag_t> tags;
    this->get_tags_in_range(
        tags, 0, this->nitems_read(0), this->nitems_read(0) + ninput_items[0]);
    auto it = get_constellation_tag(tags);
    if (it != tags.end()) {
        add_item_tag(0,
                    nitems_written(0),
                    it->key,
                    it->value);
        remove_item_tag(0, *it);
    }

    add_item_tag(0,
                    nitems_written(0),
                    pmt::string_to_symbol(d_len_tag_key),
                    pmt::from_long(n_written));

    DTL_LOG_BYTES("out:", out, n_written)

    return n_written;
}

} /* namespace dtl */
} /* namespace gr */
