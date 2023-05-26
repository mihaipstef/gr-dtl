/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_LDPC_ENC_H
#define INCLUDED_DTL_LDPC_ENC_H

#include <gnuradio/dtl/fec.h>
#include <gnuradio/fec/alist.h>
#include <gnuradio/fec/cldpc.h>

namespace gr {
namespace dtl {

class ldpc_enc: public fec_enc
{
private:
    alist d_list;
    cldpc d_code;
    std::vector<char> d_in_buf;
    std::vector<int> d_permute;

public:
    ldpc_enc(const std::string& alist_fname);
    ~ldpc_enc();

    void encode(const unsigned char* in_data, int len, unsigned char* out_data) override;
    int get_k() override;
    int get_n() override;

};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_LDPC_ENC_H*/
