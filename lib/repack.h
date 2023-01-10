/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_REPACK_H
#define INCLUDED_DTL_REPACK_H

namespace gr {
namespace dtl {

class repack
{

public:

    repack();

    int repack_lsb_first(unsigned char const* in,
                        size_t n_in,
                        unsigned char* out,
                        size_t bits_per_byte,
                        bool unpack);

    int repack_msb_first(unsigned char const* in,
                        size_t n_in,
                        unsigned char* out,
                        size_t bits_per_byte,
                        bool unpack);

    void set_indexes(unsigned char in_index, unsigned char out_index);

private:

    unsigned char d_out_index;
    unsigned char d_in_index;

};

} /* namespace dtl */
} /* namespace gr */

#endif /* INCLUDED_DTL_REPACK_H */
