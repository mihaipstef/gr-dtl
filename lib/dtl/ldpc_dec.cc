/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ldpc_dec.h"
#include <cstring>
#include <gnuradio/dtl/api.h>
#include <iostream>
#include <gnuradio/testbed/logger.h>


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ldpc_dec");

using namespace std;


vector<fec_dec::sptr> DTL_API make_ldpc_decoders(const vector<string>& alist_fnames)
{
    vector<fec_dec::sptr> decoders{nullptr};
    for (auto& fname: alist_fnames) {
        ldpc_dec::sptr dec(new ldpc_dec(fname, sqrt(2), 15));
        decoders.push_back(dec);
    }
    return decoders;
}


ldpc_dec::ldpc_dec(const std::string& alist_fname, float sigma, int max_it)
{
    DTL_LOG_DEBUG("constructor: alist={}", alist_fname);

    d_list.read(alist_fname.c_str());
    d_code.set_alist(d_list);
    //HACK: Get internal permute
    {
        stringstream new_buf;
        auto orig_buf = cout.rdbuf();
        cout.rdbuf(new_buf.rdbuf());
        d_code.print_permute();

        for (int i; new_buf >> i;) {
            d_permute.push_back(i);
            if (new_buf.peek() == ',') {
                new_buf.ignore();
            }
        }
        cout.rdbuf(orig_buf);
    }
    d_bp.set_alist(d_list);
    d_bp.set_max_iterations(max_it);
    d_bp.set_K(d_code.dimension());
    d_cw_buf.resize(d_code.get_N());
}


int ldpc_dec::decode(const float* in_data, int* nit, unsigned char* out_data)
{
    for (int i=0; i<d_code.get_N(); ++i) {
        d_cw_buf[d_permute[i]] = -1 * in_data[i];
    }
    std::vector<uint8_t> estimated(d_bp.decode(d_cw_buf, nit));
    std::vector<uint8_t> data(d_code.get_systematic_bits(estimated));
    memcpy(out_data, &data[0], d_code.dimension());
    return d_code.dimension();
}


int ldpc_dec::get_k()
{
    return d_code.dimension();
}


int ldpc_dec::get_n()
{
    return d_code.get_N();
}


} // namespace dtl
} // namespace gr
