/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ldpc_enc.h"
#include <cstring>
#include <gnuradio/dtl/api.h>
#include <iostream>
#include <gnuradio/testbed/logger.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ldpc_enc");

using namespace std;

vector<fec_enc::sptr> DTL_API make_ldpc_encoders(const vector<string>& alist_fnames)
{
    vector<fec_enc::sptr> encoders{nullptr};
    for (auto& fname: alist_fnames) {
        ldpc_enc::sptr enc(new ldpc_enc(fname));
        encoders.push_back(enc);
    }
    encoders[1]->get_k();
    return encoders;
}

ldpc_enc::ldpc_enc(const string& alist_fname) {
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
    d_in_buf.resize(d_code.dimension());
    DTL_LOG_DEBUG("constructor: buf={}, k={}, n={}", d_in_buf.size(), d_code.dimension(), d_code.get_N());
}

ldpc_enc::~ldpc_enc() {}

void ldpc_enc::encode(const unsigned char* in_data, int len, unsigned char* out_data)
{
    std::vector<unsigned char> in_buf;
    in_buf.resize(len);
    memcpy(&in_buf[0], in_data, len);
    std::vector<unsigned char> coded(d_code.encode(in_buf));
    //memcpy(&out_data[0], &coded[0], coded.size());
    for (size_t i=0; i<coded.size(); ++i) {
        out_data[i] = coded[d_permute[i]];
    }
}

int ldpc_enc::get_k()
{
    return d_code.dimension();
}

int ldpc_enc::get_n()
{
    return d_code.get_N();
}


} // namespace dtl
} // namespace gr