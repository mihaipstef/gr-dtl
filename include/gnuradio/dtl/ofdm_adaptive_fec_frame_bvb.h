/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_H

#include <gnuradio/dtl/api.h>
#include <gnuradio/dtl/fec.h>
#include <gnuradio/tagged_stream_block.h>
#include <vector>

namespace gr {
namespace dtl {

/*!
 * \brief Implement FEC layer protocol.
 * \ingroup DTL
 *
 */
class DTL_API ofdm_adaptive_fec_frame_bvb : virtual public gr::block
{
public:
    typedef std::shared_ptr<ofdm_adaptive_fec_frame_bvb> sptr;

    static sptr make(const std::vector<fec_enc::sptr> encoders,
                     int frame_capacity,
                     int max_bps,
                     const std::string& len_key);

    virtual void process_feedback(pmt::pmt_t feedback) = 0;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEC_FRAME_H */
