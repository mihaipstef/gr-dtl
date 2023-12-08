/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_MONITOR_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_MONITOR_H


#include <gnuradio/monitoring/monitor_registry.h>
#include <gnuradio/monitoring/monitor_proto.h>
#include "proto/monitor_ofdm.pb.h"

namespace gr {
namespace dtl {

struct proto_message_ids {
    static const msg_type_id_t FEC_DEC_MSG = 0;
    static const msg_type_id_t EQ_MSG = 1;
};

typedef monitor_proto<monitor_dec_msg, proto_message_ids::FEC_DEC_MSG> proto_fec_builder_t;
typedef monitor_proto<monitor_eq_msg, proto_message_ids::EQ_MSG> proto_eq_builder_t;


REGISTER_PARSERS(
    proto_fec_builder_t,
    proto_eq_builder_t)

} // namespace dtl
} // namespace gr

#endif