/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_stream_to_frame_fvf_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

using namespace std;

ofdm_adaptive_stream_to_frame_fvf::sptr ofdm_adaptive_stream_to_frame_fvf::make(int frame_capacity, const std::string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_stream_to_frame_fvf_impl>(frame_capacity, len_key);
}


/*
 * The private constructor
 */
ofdm_adaptive_stream_to_frame_fvf_impl::ofdm_adaptive_stream_to_frame_fvf_impl(int frame_capacity, const std::string& len_key)
    : gr::block(
          "ofdm_adaptive_stream_to_frame_fvf",
          gr::io_signature::make(
              1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
          gr::io_signature::make(
              1 /* min outputs */, 1 /*max outputs */, frame_capacity * sizeof(float))),
    d_frame_capacity(frame_capacity),
    d_len_key(pmt::string_to_symbol(len_key))
{
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_stream_to_frame_fvf_impl::~ofdm_adaptive_stream_to_frame_fvf_impl() {}



int ofdm_adaptive_stream_to_frame_fvf_impl::general_work(int noutput_items,
                                                 gr_vector_int& ninput_items,
                                                 gr_vector_const_void_star& input_items,
                                                 gr_vector_void_star& output_items)
{
    auto in = static_cast<const float*>(input_items[0]);
    auto out = static_cast<float*>(output_items[0]);

    vector<tag_t> tags;
    get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + 1);
    for (auto& tag: tags) {
        if (tag.key == d_len_key) {
            assert(pmt::to_long(tag.value) == d_frame_capacity);
            break;
        }
    }

    if (d_frame_capacity > ninput_items[0]) {
        return 0;
    }

    int nframes = min(ninput_items[0] / d_frame_capacity, noutput_items);

    for (int i=0; i<nframes; ++i) {
        memcpy(&out[i], &in[i*d_frame_capacity], d_frame_capacity);
        for (auto& tag: tags) {
            add_item_tag(0, nitems_written(0) + i, tag.key, tag.value);
        }
    }

    consume_each(nframes * d_frame_capacity);

    return nframes;
}

} /* namespace dtl */
} /* namespace gr */
