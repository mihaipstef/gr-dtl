/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_CONSTELLATION_H
#define INCLUDED_DTL_CONSTELLATION_H

#include <gnuradio/digital/constellation.h>

namespace gr {
namespace dtl {

class constellation_qpsk_normalized : public gr::digital::constellation_qpsk
{
public:
    typedef std::shared_ptr<constellation_qpsk_normalized> sptr;

    static sptr make();

    ~constellation_qpsk_normalized() override;

protected:
    constellation_qpsk_normalized();

private:
    double d_factor;

};

} // namespace dtl
} // namespace gr

#endif