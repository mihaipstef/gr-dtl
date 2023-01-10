/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
namespace dtl {

/*!
 * \brief Tag the input stream according to the feedback received.
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_tx_control_bb : virtual public gr::tagged_stream_block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_tx_control_bb> sptr;

    static sptr
    make(const std::string& len_tag_key, size_t frame_len, size_t n_payload_carriers);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_TX_CONTROL_BB_H */