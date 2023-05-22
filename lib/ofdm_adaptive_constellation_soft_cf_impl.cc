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


static int SAFETY_COUNT = 0;

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
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_constellation_soft_cf_impl::~ofdm_adaptive_constellation_soft_cf_impl() {}


void ofdm_adaptive_constellation_soft_cf_impl::forecast(int noutput_items,
                                   gr_vector_int& ninput_items_required)
{
    ninput_items_required[0] = std::max(
            1, (int)std::floor((double)noutput_items / relative_rate() + 0.5));
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

    while (read_index < ninput_items[0]) {

        std::vector<tag_t> tags = {};
        constellation_type_t cnst = constellation_type_t::UNKNOWN;
        int len = 0;
        this->get_tags_in_range(tags,
                                0,
                                this->nitems_read(0) + read_index,
                                this->nitems_read(0) + read_index + 1);
        for (auto& tag: tags)
        {
            DTL_LOG_DEBUG("o={}, key: {}",tag.offset, pmt::symbol_to_string(tag.key));
        }
        int test = 0;
        for (auto& tag : tags) {
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
            throw std::runtime_error("missing tags");
        }

        d_constellation = d_constellations[cnst];
        int bps = d_constellation->bits_per_symbol();
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

        for (int i=0; i < len; ++i, ++read_index, write_index += bps) {
            std::vector<float> llrs(d_constellation->calc_soft_dec(in[read_index], 1));
            memcpy(&out[write_index], &llrs[0], sizeof(float) * llrs.size());
        }

        // for (auto& tag : tags) {
        //     add_item_tag(0, d_tag_offset, tag.key, tag.value);
        // }

        //add_item_tag(0, d_tag_offset, d_len_key, pmt::from_long(len));

        d_tag_offset += len * bps;
    }

    // if (!write_index && !read_index) {
    //     ++SAFETY_COUNT;
    //     if (SAFETY_COUNT > 20) {
    //         return WORK_DONE;
    //     }
    // }

    DTL_LOG_DEBUG("work: produced={}, consumed={}",write_index, read_index);
    if (read_index>0) consume_each(read_index);
    return write_index;
}

} /* namespace dtl */
} /* namespace gr */
