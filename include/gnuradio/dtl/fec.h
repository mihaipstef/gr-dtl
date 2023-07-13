/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_FEC_H
#define INCLUDED_DTL_FEC_H

#include <memory>
#include <vector>

namespace gr {
namespace dtl {

class fec_enc {
public:
    typedef std::shared_ptr<fec_enc> sptr;
    virtual void encode(const unsigned char* in_data, int len, unsigned char* out_data) = 0;
    virtual int get_k() = 0;
    virtual int get_n() = 0;
};

class fec_dec {
public:
    typedef std::shared_ptr<fec_dec> sptr;
    virtual int decode(const float* in_data, int *nit, unsigned char* out_data) = 0;
    virtual int get_k() = 0;
    virtual int get_n() = 0;
};

std::vector<fec_enc::sptr> make_ldpc_encoders(const std::vector<std::string>& alist_fnames);

std::vector<fec_dec::sptr> make_ldpc_decoders(const std::vector<std::string>& alist_fnames);


} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_FEC_H*/