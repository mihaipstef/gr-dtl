/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_constellation_decoder_cb_impl.h"

#include <gnuradio/io_signature.h>


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_constellation_decoder_cb");

using namespace gr::digital;

using input_type = gr_complex;
using output_type = unsigned char;


ofdm_adaptive_constellation_decoder_cb::sptr ofdm_adaptive_constellation_decoder_cb::make(
    const std::vector<constellation_type_t>& constellations,
    const std::string& tsb_tag_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_constellation_decoder_cb_impl>(
        constellations, tsb_tag_key);
}


ofdm_adaptive_constellation_decoder_cb_impl::ofdm_adaptive_constellation_decoder_cb_impl(
    const std::vector<constellation_type_t>& constellations,
    const std::string& tsb_tag_key)
    : gr::tagged_stream_block(
          "ofdm_adaptive_constellation_decoder_cb",
          gr::io_signature::make(
              1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
          gr::io_signature::make(
              1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)),
          tsb_tag_key)
{
    // Populate known constellations
    for (const auto& constellation_type : constellations) {
        auto constellation = create_constellation(constellation_type);
        if (constellation == nullptr) {
            throw std::invalid_argument("Unknown constellation");
        }
        d_constellations[constellation_type] = constellation;
    }
}


ofdm_adaptive_constellation_decoder_cb_impl::
    ~ofdm_adaptive_constellation_decoder_cb_impl()
{
}


int ofdm_adaptive_constellation_decoder_cb_impl::calculate_output_stream_length(
    const gr_vector_int& ninput_items)
{
    DTL_LOG_DEBUG("calculate_output_stream_length: {}", ninput_items[0]);
    return ninput_items[0];
}


int ofdm_adaptive_constellation_decoder_cb_impl::work(
    int noutput_items,
    gr_vector_int& ninput_items,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    auto in = static_cast<const input_type*>(input_items[0]);
    auto out = static_cast<output_type*>(output_items[0]);

    std::vector<tag_t> tags;
    this->get_tags_in_range(
        tags, 0, this->nitems_read(0), this->nitems_read(0) + ninput_items[0]);
    constellation_type_t constellation_type = find_constellation_type(tags);
    if (constellation_type_t::UNKNOWN == constellation_type) {
        throw std::invalid_argument("Constellation type not found in tags");
    }

    constellation_sptr constellation = d_constellations[constellation_type];
    for (int i = 0; i < ninput_items[0]; ++i) {
        *out = constellation->decision_maker(in);
        ++in;
        ++out;
    }
    return ninput_items[0];
}

} /* namespace dtl */
} /* namespace gr */
