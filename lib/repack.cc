/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "repack.h"

namespace gr {
namespace dtl {

repack::repack(): d_out_index(0), d_in_index(0)
{
}

int repack::repack_lsb_first(unsigned char const* in,
                     size_t n_in,
                     unsigned char* out,
                     size_t bits_per_byte,
                     bool unpack)
{
    int bits_per_in_byte = bits_per_byte;
    int bits_per_out_byte = 8;
    if (unpack) {
        bits_per_in_byte = 8;
        bits_per_out_byte = bits_per_byte;
    }

    size_t bytes_to_write = n_in * bits_per_in_byte / bits_per_out_byte;

    if (((n_in * bits_per_in_byte) % bits_per_out_byte) != 0) {
        bytes_to_write += static_cast<int>(unpack);
    }

    size_t n_read = 0;
    size_t n_written = 0;

    while (n_written < bytes_to_write && n_read < n_in) {
        if (d_out_index == 0) {
            out[n_written] = 0;
        }
        out[n_written] |= ((in[n_read] >> d_in_index) & 0x01) << d_out_index;

        d_in_index = (d_in_index + 1) % bits_per_in_byte;
        d_out_index = (d_out_index + 1) % bits_per_out_byte;
        if (d_in_index == 0) {
            n_read++;
        }
        if (d_out_index == 0) {
            n_written++;
        }
    }

    if (d_out_index) {
        n_written++;
        d_out_index = 0;
    }
    return n_written;
}


int repack::repack_msb_first(unsigned char const* in,
                     size_t n_in,
                     unsigned char* out,
                     size_t bits_per_byte,
                     bool unpack)
{
    int bits_per_in_byte = bits_per_byte;
    int bits_per_out_byte = 8;
    if (unpack) {
        bits_per_in_byte = 8;
        bits_per_out_byte = bits_per_byte;
    } else {
    }

    size_t bytes_to_write = n_in * bits_per_in_byte / bits_per_out_byte;

    if (((n_in * bits_per_in_byte) % bits_per_out_byte) != 0) {
        bytes_to_write += static_cast<int>(unpack);
    }

    size_t n_read = 0;
    size_t n_written = 0;

    while (n_written < bytes_to_write && n_read < n_in) {
        if (d_out_index == 0) {
            out[n_written] = 0;
        }
        out[n_written] |= ((in[n_read] >> (bits_per_in_byte - 1 - d_in_index)) & 0x01)
                          << (bits_per_out_byte - 1 - d_out_index);

        d_in_index = (d_in_index + 1) % bits_per_in_byte;
        d_out_index = (d_out_index + 1) % bits_per_out_byte;
        if (d_in_index == 0) {
            n_read++;
        }
        if (d_out_index == 0) {
            n_written++;
        }
    }

    if (d_out_index) {
        n_written++;
        d_out_index = 0;
    }

    return n_written;
}


void repack::set_indexes(unsigned char in_index, unsigned char out_index)
{
    d_out_index = (out_index >= 0) ? out_index : d_out_index;
    d_in_index = (in_index >= 0) ? in_index : d_in_index;
}


} /* namespace dtl */
} /* namespace gr */