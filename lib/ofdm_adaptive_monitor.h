/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_MONITOR_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_MONITOR_H


#include <gnuradio/dtl/monitor_registry.h>
#include <gnuradio/dtl/monitor_proto.h>
#include "proto/monitor_fec.pb.h"

namespace gr {
namespace dtl {

struct proto_message_ids {
    static const msg_type_id_t FEC_DEC_MSG = 0;
};

typedef monitor_proto<monitor_dec_msg, proto_message_ids::FEC_DEC_MSG> proto_fec_builder_t;

typedef message_registry<proto_fec_builder_t> ofdm_adaptive_monitor_registry_t;

} // namespace dtl
} // namespace gr

#endif