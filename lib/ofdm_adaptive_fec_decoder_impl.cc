/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_fec_decoder_impl.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

using namespace std;

typedef tuple<unsigned char, unsigned char, int, unsigned char, unsigned char> fec_header_tuple_t;

ofdm_adaptive_fec_decoder::sptr ofdm_adaptive_fec_decoder::make(const string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_fec_decoder_impl>(len_key);
}

ofdm_adaptive_fec_decoder_impl::ofdm_adaptive_fec_decoder_impl(const string& len_key)
    : gr::tagged_stream_block(
          "ofdm_adaptive_fec_decoder",
          gr::io_signature::make(
              1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
          gr::io_signature::make(
              1 /* min outputs */, 1 /*max outputs */, sizeof(unsigned char)),
          len_key),
    d_buffers(2)
{
    // Preallocate TB buffers to max TB len
}

ofdm_adaptive_fec_decoder_impl::~ofdm_adaptive_fec_decoder_impl() {}

void ofdm_adaptive_fec_decoder_impl::fill_shortened(int tb_len, int n_cw, int k, int n)
{
    static float SHORTENED_VALUE = 0;
    for (int i=0, read_index=0; i<n_cw; ++i) {
        int remaining = tb_len - read_index;
        int _k = (remaining)/(n_cw - i) + static_cast<int>((remaining % (n_cw-i)) > 0);
        memcpy(&d_buffers[1][i*n], &d_buffers[0][read_index], _k);
        read_index += _k;
        memset(&d_buffers[1][i*n+_k], SHORTENED_VALUE, n-_k);
        memcpy(&d_buffers[1][i*n+k], &d_buffers[0][read_index], n-k);
        read_index += n-k;
    }
}

int ofdm_adaptive_fec_decoder_impl::decode(unsigned char* out, int tb_payload_len, int n_cw, int fec_idx)
{
    fec_dec::sptr dec = d_decoders[fec_idx];

    int it = 10;
    for (int i=0; i<n_cw; ++i) {
        dec->decode(&d_buffers[1][i*dec->get_n()], &it, out);
    }

    d_ready_to_decode = false;
}

// void ofdm_adaptive_fec_decoder_impl::parse_length_tags(const std::vector<std::vector<tag_t>>& tags,
//                                             gr_vector_int& n_input_items_reqd)
// {
//     for (unsigned k = 0; k < tags[i].size(); k++) {
//         if (tags[i][k].key == fec_tb_len_key {
//             n_input_items_reqd[i] = pmt::to_long(tags[i][k].value);
//             remove_item_tag(i, tags[i][k]);
//         }
//     }
// }

int ofdm_adaptive_fec_decoder_impl::work(int noutput_items,
                                         gr_vector_int& ninput_items,
                                         gr_vector_const_void_star& input_items,
                                         gr_vector_void_star& output_items)
{
    auto in = static_cast<const float*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    int write_index = 0;
    int read_index = 0;

    // If TB buffer already full...
    if (d_ready_to_decode) {
        // ... decode if output available.
        if (noutput_items - write_index >= d_tb_payload_len) {
            decode(&out[write_index], d_tb_payload_len, d_n_cw, d_fec_idx);
            write_index += d_tb_payload_len;
        }
    } else {
        // Fill the TB buffers
        while (read_index < ninput_items[0]) {
            // Find next FEC tags group and get values
            vector<tag_t> tags;
            get_tags_in_window(tags, 0, read_index, read_index + d_frame_len);

            int tags_check = 0;
            for (auto& tag: tags) {
                if (tag.key == fec_key()) {
                    tags_check &= 1;
                    d_last_fec_idx = d_fec_idx;
                    d_fec_idx = pmt::to_long(tag.value);
                } else if (tag.key == fec_tb_key()) {
                    tags_check &= 2;
                    int tb_key = pmt::to_long(tag.value);
                    d_current_tb_id = tb_key & 0xff;
                    d_previous_tb_id = (tb_key >> 8) & 0xff;
                } else if (tag.key == fec_offset_key()) {
                    tags_check &= 4;
                    d_tb_offset = pmt::to_long(tag.value);
                } else if (tag.key == fec_tb_index_key()) {
                    tags_check &= 8;
                    d_tb_index = pmt::to_long(tag.value);
                } else if (tag.key == payload_length_key()) {
                    d_frame_payload_len = pmt::to_long(tag.value);
                    tags_check &= 16;
                } else if (tag.key == fec_tb_payload_key()) {
                    d_tb_payload_len = pmt::to_long(tag.value);
                    tags_check &= 32;
                } else if (tag.key == get_constellation_tag_key()) {
                    int bps = get_bits_per_symbol(static_cast<constellation_type_t>(
                        pmt::to_long(tag.value)
                    ));
                    d_frame_len = d_frame_len_syms * bps;
                    tags_check &= 64;
                }
                if ((tags_check ^ 0x7F) == 0) {
                    break;
                }
            }

            assert((tags_check ^ 0x7F) == 0 || tags_check == 0);

            int tb_id_diff = (d_current_tb_id - d_last_current_tb_id) % 256;
            bool new_tb = d_current_tb_id != d_previous_tb_id && (tags_check ^ 0x3F) == 0;
            d_n_cw = 1;
            if (d_frame_len > d_decoders[d_fec_idx]->get_n()) {
                d_n_cw = 2 * d_frame_len / d_decoders[d_fec_idx]->get_n();
            }

            if(tb_id_diff > 1 && new_tb) {
                // Start re-filliing current buffer (current TB is compromised)
                d_tb_buffer_idx = 0;
                int to_copy = min(d_frame_payload_len - d_tb_offset, ninput_items[0] - read_index);
                memcpy(&d_buffers[0][d_tb_buffer_idx], &in[read_index], to_copy);
                read_index += to_copy;
                d_tb_buffer_idx += to_copy;
                d_tb_compromised = false;
            } else if (tb_id_diff == 1) {
                if (new_tb) {
                    if (!d_tb_compromised) {
                        // Fill the rest of the current buffer
                        memcpy(&d_buffers[0][d_tb_buffer_idx], &in[read_index], d_tb_offset);
                        read_index += d_tb_offset;
                        // Fill shortened information
                        fill_shortened(d_tb_payload_len, d_n_cw, d_decoders[d_last_fec_idx]->get_k(), d_decoders[d_last_fec_idx]->get_n());
                        if (noutput_items - write_index >= d_tb_payload_len) {
                            decode(&out[write_index], d_tb_payload_len, d_n_cw, d_fec_idx);
                            write_index += d_tb_payload_len;
                        } else {
                            d_ready_to_decode = true;
                        }
                    }
                    // Start fillling the buffer with next TB
                    d_tb_buffer_idx = 0;
                    int to_copy = d_frame_payload_len - d_tb_offset;
                    memcpy(&d_buffers[0][d_tb_buffer_idx], &in[read_index], to_copy);
                    read_index += to_copy;
                    d_tb_buffer_idx += to_copy;
                    d_tb_compromised = false;
                }
            } else if(tb_id_diff == 0) {

                assert(d_fec_idx == d_last_fec_idx);

                if (d_tb_index - d_last_tb_index == 1) {
                    // copy data to current tb buffer using current tb index
                    memcpy(&d_buffers[0][d_tb_buffer_idx], &in[read_index], d_frame_payload_len);
                    read_index += d_frame_payload_len;
                    d_tb_buffer_idx += d_frame_payload_len;
                } else {
                    // lost frame because of sync and/or header error
                    d_tb_compromised = true;
                }
            }
        }
    }

    consume_each(read_index);
    return write_index;
}

} /* namespace dtl */
} /* namespace gr */
