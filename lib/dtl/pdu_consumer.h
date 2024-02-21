/* -*- c++ -*- */
/*
 * Copyright 2024 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_PDU_CONSUMER_H
#define INCLUDED_DTL_PDU_CONSUMER_H

#include <functional>
#include <gnuradio/tags.h>
#include <optional>
#include <pmt/pmt.h>

namespace gr {
namespace dtl {


class pdu_consumer
{
private:
    int d_current_pdu_remain;
public:
    pdu_consumer();
    int advance(int ninput_items, int payload_max, int start_offset, const std::function<std::optional<tag_t>(int)>& get_len_tag);
};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_LDPC_ENC_H*/
