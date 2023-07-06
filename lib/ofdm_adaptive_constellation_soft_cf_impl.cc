/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_constellation_soft_cf_impl.h"
#include <gnuradio/io_signature.h>
#include "logger.h"

namespace gr {
namespace dtl {


using namespace digital;


INIT_DTL_LOGGER("ofdm_adaptive_constellation_soft_cf");


ofdm_adaptive_constellation_soft_cf::sptr ofdm_adaptive_constellation_soft_cf::make(
    const std::vector<constellation_type_t>& constellations, const std::string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_constellation_soft_cf_impl>(
        constellations, len_key);
}


/*
 * The private constructor
 */
ofdm_adaptive_constellation_soft_cf_impl::ofdm_adaptive_constellation_soft_cf_impl(
    const std::vector<constellation_type_t>& constellations, const std::string& len_key)
    : gr::block(
          "ofdm_adaptive_constellation_soft_cf",
          gr::io_signature::make(
              1 /* min inputs */, 1 /* max inputs */, sizeof(gr_complex)),
          gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(float))),
      d_len_key(pmt::string_to_symbol(len_key)),
      d_tag_offset(0)
{
    // Populate known constellations
    for (const auto& constellation_type : constellations) {
        auto constellation = create_constellation(constellation_type);
        if (constellation == nullptr) {
            throw std::invalid_argument("Unknown constellation");
        }
        d_constellations[constellation_type] = constellation;
    }
    set_max_noutput_items(65536);
    set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_constellation_soft_cf_impl::~ofdm_adaptive_constellation_soft_cf_impl() {}


void ofdm_adaptive_constellation_soft_cf_impl::forecast(int noutput_items,
                                   gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = 1;
}


int ofdm_adaptive_constellation_soft_cf_impl::general_work(
    int noutput_items,
    gr_vector_int& ninput_items,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    auto in = static_cast<const gr_complex*>(input_items[0]);
    auto out = static_cast<float*>(output_items[0]);

    int read_index = 0;
    int write_index = 0;
    DTL_LOG_DEBUG("work: ninput={}, noutput={}", ninput_items[0], noutput_items);
    while (read_index < ninput_items[0]) {
        std::vector<tag_t> tags = {};
        constellation_type_t cnst = constellation_type_t::UNKNOWN;
        int len = 0;
        this->get_tags_in_range(tags,
                                0,
                                this->nitems_read(0) + read_index,
                                this->nitems_read(0) + read_index + 1);
        int test = 0;
        for (auto& tag : tags) {
            DTL_LOG_DEBUG("offset={}, key={}, value={}", tag.offset, pmt::symbol_to_string(tag.key), pmt::to_long(tag.value));
            if (tag.key == get_constellation_tag_key()) {
                cnst = static_cast<constellation_type_t>(pmt::to_long(tag.value));
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
        if (test != 3) {
            DTL_LOG_ERROR("Tags missing: check_bitmap={}, lookup_offset={}", test, nitems_read(0) + read_index);
            throw std::runtime_error("Tags missing");
        }

        d_constellation = d_constellations[cnst];
        int bps = d_constellation->bits_per_symbol();

        DTL_LOG_DEBUG("len_key={}, val={}, offset={}", pmt::symbol_to_string(d_len_key), pmt::from_long(len * bps), nitems_written(0) + write_index);

        set_relative_rate(bps, 1);
        set_output_multiple(len * bps);

        DTL_LOG_DEBUG("work: ninput={}, noutput={}, cnst={}, len={}, bps={}", ninput_items[0], noutput_items, (int)cnst, len, bps);

        if (read_index + len > ninput_items[0]) {
            break;
        }

        if (write_index + bps * len > noutput_items) {            
            int nout = bps * len;
            DTL_LOG_DEBUG("min_out={}", nout);
            set_min_noutput_items(nout);
            break;
        }
        set_min_noutput_items(1);

        for (auto& tag: tags) {
            if (tag.key == d_len_key) {
                add_item_tag(0, nitems_written(0) + write_index, d_len_key, pmt::from_long(len * bps));
            } else {
                add_item_tag(0, nitems_written(0) + write_index, tag.key, tag.value);
            }
        }

        for (int i=0; i < len; ++i, ++read_index, write_index += bps) {
            std::vector<float> llrs(d_constellation->calc_soft_dec(in[read_index], 0.1));
            // TODO: Use MSB order to avoid reversing here
            std::reverse(llrs.begin(), llrs.end());
            memcpy(&out[write_index], &llrs[0], sizeof(float) * llrs.size());
        }
        d_tag_offset += len * bps;
    }


    DTL_LOG_DEBUG("work: produced={}, consumed={}",write_index, read_index);
    if (read_index>0) consume_each(read_index);
    return write_index;
}

} /* namespace dtl */
} /* namespace gr */
