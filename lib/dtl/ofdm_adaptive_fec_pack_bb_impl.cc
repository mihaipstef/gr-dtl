/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_fec_pack_bb_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/testbed/logger.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_fec_pack_bb");

ofdm_adaptive_fec_pack_bb::sptr ofdm_adaptive_fec_pack_bb::make(const std::string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_fec_pack_bb_impl>(len_key);
}

/*
 * The private constructor
 */
ofdm_adaptive_fec_pack_bb_impl::ofdm_adaptive_fec_pack_bb_impl(const std::string& len_key)
    : gr::block("ofdm_adaptive_fec_pack_bb",
                gr::io_signature::make(
                    1 /* min inputs */, 1 /* max inputs */, sizeof(char)),
                gr::io_signature::make(
                    1 /* min outputs */, 1 /*max outputs */, sizeof(char))),
    d_len_key(pmt::string_to_symbol(len_key)),
    packer(1, 8)
{
    set_tag_propagation_policy(block::tag_propagation_policy_t::TPP_DONT);
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_fec_pack_bb_impl::~ofdm_adaptive_fec_pack_bb_impl() {}


void ofdm_adaptive_fec_pack_bb_impl::forecast(int noutput_items,
                                   gr_vector_int& ninput_items_required) {
    ninput_items_required[0] = 8; // minimum 1 byte
}

int ofdm_adaptive_fec_pack_bb_impl::general_work(int noutput_items,
                                                 gr_vector_int& ninput_items,
                                                 gr_vector_const_void_star& input_items,
                                                 gr_vector_void_star& output_items)
{
    auto in = static_cast<const unsigned char*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    // Pack all bytes we can
    int to_pack = std::min(8*(ninput_items[0]/8), noutput_items*8);

    int nbytes = packer.repack_lsb_first(in, to_pack, out, true);

    DTL_LOG_DEBUG("to_pack={}, ninput={}, noutput={}", to_pack, ninput_items[0], noutput_items);

    assert(nbytes == to_pack/8 && to_pack % 8 == 0);

    add_item_tag(0, nitems_written(0), d_len_key, pmt::from_long(nbytes));

    consume_each(to_pack);
    return nbytes;
}

} /* namespace dtl */
} /* namespace gr */
