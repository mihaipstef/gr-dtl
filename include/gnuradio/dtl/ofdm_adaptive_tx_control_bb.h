/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief Tag the input stream according to the feedback received.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_tx_control_bb : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_tx_control_bb> sptr;

    static sptr make(const std::string& len_tag_key, size_t packet_len);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H */