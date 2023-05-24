/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_fec_decoder_impl.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>
#include "fec_utils.h"
#include "logger.h"

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_fec_decoder");


using namespace std;

typedef tuple<unsigned char, unsigned char, int, unsigned char, unsigned char>
    fec_header_tuple_t;

ofdm_adaptive_fec_decoder::sptr ofdm_adaptive_fec_decoder::make(
    const vector<fec_dec::sptr> decoders, int frame_capacity, int max_bps, const string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_fec_decoder_impl>(decoders, frame_capacity, max_bps, len_key);
}

ofdm_adaptive_fec_decoder_impl::ofdm_adaptive_fec_decoder_impl(const vector<fec_dec::sptr> decoders, int frame_capacity, int max_bps, const string& len_key)
    : gr::block(
          "ofdm_adaptive_fec_decoder",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
          gr::io_signature::make(
              1 /* min outputs */, 1 /*max outputs */, sizeof(unsigned char))),
      d_len_key(pmt::string_to_symbol(len_key)),
      d_decoders(decoders),
      d_frame_capacity(frame_capacity),
      d_data_ready(false)
{
    DTL_LOG_DEBUG("constructor");
    auto it = max_element(d_decoders.begin() + 1,
                          d_decoders.end(),
                          [](const decltype(d_decoders)::value_type& l,
                             const decltype(d_decoders)::value_type& r) {
                              return l->get_n() < r->get_n();
                          });
    if (it == d_decoders.end()) {
        throw(std::runtime_error("No encoder found!"));
    }
    int frame_len = d_frame_capacity * max_bps;
    d_tb_dec = make_shared<tb_decoder>(frame_len *
                                           compute_tb_len((*it)->get_n(), frame_len));
}

ofdm_adaptive_fec_decoder_impl::~ofdm_adaptive_fec_decoder_impl() {}


int ofdm_adaptive_fec_decoder_impl::general_work(int noutput_items,
                                         gr_vector_int& ninput_items,
                                         gr_vector_const_void_star& input_items,
                                         gr_vector_void_star& output_items)
{
    auto in = static_cast<const float*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    int nframes = 0;
    int read_index = 0;
    int write_index = 0;
    int frame_len = 0;
    DTL_LOG_DEBUG("work: ninput={}, noutput={}", ninput_items[0], noutput_items);
    while (read_index < ninput_items[0]) {
        vector<tag_t> tags;
        get_tags_in_range(tags, 0, nitems_read(0) + read_index, nitems_read(0) + read_index + 1);

        int test = 0;
        int bps = 0;
        int len = 0;
        for (auto& tag : tags) {
            if (tag.key == get_constellation_tag_key()) {
                bps = get_bits_per_symbol(static_cast<constellation_type_t>(pmt::to_long(tag.value)));
                test |= 1;
            } else if (tag.key == d_len_key) {
                len = pmt::to_long(tag.value);
                test |= 2;
                //remove_item_tag(0, tag);
            }
            if (test == 3) {
                break;
            }
        }
        frame_len = len * bps;

        if (read_index + frame_len > ninput_items[0]) {
            break;
        }

        for (auto& tag: tags)
        {
            DTL_LOG_DEBUG("o={}, key={}, val={}",tag.offset, pmt::symbol_to_string(tag.key), pmt::to_long(tag.value));
        }

        fec_info_t::sptr fec_info = make_fec_info(tags, {}, d_decoders);
    
        if (!fec_info) {
            throw runtime_error("FEC tags missing");
        }

        fec_info->d_tb_offset *= bps;

        if (!d_data_ready) {
            d_data_ready = d_tb_dec->process_frame(&in[read_index], frame_len, fec_info);
            read_index += frame_len;
        }

        if (d_data_ready) {
            if (noutput_items < fec_info->d_tb_payload_len) {
                break;
            }
            const vector<unsigned char>& data = d_tb_dec->data();
            memcpy(&out[write_index], &data[0], data.size());
            write_index += data.size();
            d_data_ready = false;
        }
    }
    consume_each(read_index);
    return write_index;
}

} /* namespace dtl */
} /* namespace gr */
