/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_chunks_to_symbols_bc_impl.h"

#include "logger.h"

#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_chunks_to_symbols");

using namespace gr::digital;


typedef unsigned char input_type;
typedef gr_complex output_type;


ofdm_adaptive_chunks_to_symbols_bc::sptr ofdm_adaptive_chunks_to_symbols_bc::make(
    const std::vector<constellation_type_t>& constellations,
    const std::string& tsb_tag_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_chunks_to_symbols_bc_impl>(
        constellations, tsb_tag_key);
}


ofdm_adaptive_chunks_to_symbols_bc_impl::ofdm_adaptive_chunks_to_symbols_bc_impl(
    const std::vector<constellation_type_t>& constellations,
    const std::string& tsb_tag_key)
    : gr::tagged_stream_block(
          "ofdm_adaptive_chunks_to_symbols_bc",
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

ofdm_adaptive_chunks_to_symbols_bc_impl::~ofdm_adaptive_chunks_to_symbols_bc_impl() {}


int ofdm_adaptive_chunks_to_symbols_bc_impl::work(int noutput_items,
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
        constellation->map_to_points(*in, out);
        ++in;
        ++out;
    }
    DTL_LOG_DEBUG("size:{}, constellation: {}",ninput_items[0], (int)constellation_type);

    return ninput_items[0];
}

} /* namespace dtl */
} /* namespace gr */
