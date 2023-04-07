/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_REPACK_H
#define INCLUDED_DTL_REPACK_H

#include <iomanip>

namespace gr {
namespace dtl {

class repack
{

public:
    repack(unsigned char bits_in_byte, unsigned char bits_out_byte);

    void set_bits_per_byte(unsigned char bits_in_byte, unsigned char bits_out_byte);

    int repack_lsb_first(unsigned char const* in,
                         size_t n_in,
                         unsigned char* out,
                         bool unpack);

    int repack_msb_first(unsigned char const* in,
                         size_t n_in,
                         unsigned char* out,
                         bool unpack);

    void set_indexes(unsigned char in_index, unsigned char out_index);

private:
    unsigned char d_out_index;
    unsigned char d_in_index;
    unsigned char d_bits_in_byte;
    unsigned char d_bits_out_byte;
};

} /* namespace dtl */
} /* namespace gr */

#endif /* INCLUDED_DTL_REPACK_H */
