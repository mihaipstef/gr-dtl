/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_frame_to_stream_vbb_impl.h"
#include <gnuradio/io_signature.h>
#include "logger.h"


namespace gr {
namespace dtl {


using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_frame_to_stream_vbb");

ofdm_adaptive_frame_to_stream_vbb::sptr ofdm_adaptive_frame_to_stream_vbb::make(int frame_capacity, std::string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_to_stream_vbb_impl>(frame_capacity, len_key);
}

/*
 * The private constructor
 */
ofdm_adaptive_frame_to_stream_vbb_impl::ofdm_adaptive_frame_to_stream_vbb_impl(int frame_capacity, std::string& len_key)
    : gr::block("ofdm_adaptive_frame_to_stream_vbb",
                gr::io_signature::make(
                    1 /* min inputs */, 1 /* max inputs */, frame_capacity * sizeof(char)),
                gr::io_signature::make(
                    1 /* min outputs */, 1 /*max outputs */, sizeof(char))),
                    d_len_key(pmt::string_to_symbol(len_key)),
                    d_frame_capacity(frame_capacity)
{
    set_fixed_rate(true);
    set_relative_rate(d_frame_capacity, 1);
    set_output_multiple(d_frame_capacity);
}

int ofdm_adaptive_frame_to_stream_vbb_impl::fixed_rate_ninput_to_noutput(int ninput_items)
{
    return ninput_items * d_frame_capacity;
}

int ofdm_adaptive_frame_to_stream_vbb_impl::fixed_rate_noutput_to_ninput(int noutput_items)
{
    return noutput_items / d_frame_capacity;
}

void ofdm_adaptive_frame_to_stream_vbb_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
    unsigned ninputs = ninput_items_required.size();
    for (unsigned i = 0; i < ninputs; i++)
        ninput_items_required[i] = fixed_rate_noutput_to_ninput(noutput_items);
}


/*
 * Our virtual destructor.
 */
ofdm_adaptive_frame_to_stream_vbb_impl::~ofdm_adaptive_frame_to_stream_vbb_impl() {}


int ofdm_adaptive_frame_to_stream_vbb_impl::general_work(
    int noutput_items,
    gr_vector_int& ninput_items,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    auto in = static_cast<const char*>(input_items[0]);
    auto out = static_cast<char*>(output_items[0]);

    int nframes = min(ninput_items[0], noutput_items / d_frame_capacity);

    DTL_LOG_DEBUG("noutput={}, frames={}", noutput_items, nframes);
    vector<tag_t> tags;
    for (int i=0; i<nframes; ++i) {
        // get tags
        get_tags_in_window(tags, 0, i, i+1);
        // copy input frame to output
        memcpy(&out[i*d_frame_capacity], &in[i], d_frame_capacity);

        // set new tag offset
        for (auto& tag: tags) {
            add_item_tag(0, nitems_written(0) + i * d_frame_capacity, tag.key, tag.value);
        }
    }

    consume_each(nframes);

    return nframes * d_frame_capacity;
}

} /* namespace dtl */
} /* namespace gr */
