/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_LDPC_DEC_H
#define INCLUDED_DTL_LDPC_DEC_H

#include <gnuradio/dtl/fec.h>
#include <gnuradio/fec/alist.h>
#include <gnuradio/fec/awgn_bp.h>
#include <gnuradio/fec/cldpc.h>

namespace gr {
namespace dtl {

class ldpc_dec: public fec_dec
{
private:

    alist d_list;
    cldpc d_code;
    awgn_bp d_bp;
    std::vector<float> d_cw_buf;
    std::vector<int> d_permute;


public:
    ldpc_dec(const std::string& alist_fname, int max_it);
    int decode(const float* in_data, int *nit, unsigned char* out_data) override;
    int get_k() override;
    int get_n() override;

};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_LDPC_DEC_H*/
