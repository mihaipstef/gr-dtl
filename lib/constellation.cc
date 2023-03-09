/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "constellation.h"

namespace gr {
namespace dtl {

constellation_qpsk_normalized::sptr constellation_qpsk_normalized::make()
{
    return constellation_qpsk_normalized::sptr(new constellation_qpsk_normalized());
}

constellation_qpsk_normalized::constellation_qpsk_normalized()
    : gr::digital::constellation_qpsk(), d_factor(0.5)
{
    for (auto& c: d_constellation) {
        c = gr_complex(c.real() * d_factor, c.imag() * d_factor);
    }
}

constellation_qpsk_normalized::~constellation_qpsk_normalized() {}


} // namespace dtl
} // namespace gr