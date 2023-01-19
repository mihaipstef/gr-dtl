/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_IMPL_H

#include <gnuradio/dtl/ofdm_adaptive_chunks_to_symbols_bc.h>

namespace gr {
namespace dtl {

class ofdm_adaptive_chunks_to_symbols_bc_impl : public ofdm_adaptive_chunks_to_symbols_bc
{
private:
    constellation_dictionary_t d_constellations;

public:
    ofdm_adaptive_chunks_to_symbols_bc_impl(
        const std::vector<constellation_type_t>& constellations,
        const std::string& tsb_tag_key);
    ~ofdm_adaptive_chunks_to_symbols_bc_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_int& ninput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_CHUNKS_TO_SYMBOLS_BC_IMPL_H */
