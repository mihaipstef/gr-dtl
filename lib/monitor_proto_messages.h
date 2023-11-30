/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_PROTO_MESSAGES_H
#define INCLUDED_DTL_MONITOR_PROTO_MESSAGES_H


#include <gnuradio/dtl/monitor_proto.h>
#include "proto/monitor_fec.pb.h"

namespace gr {
namespace dtl {


struct proto_messages {
    static const msg_type_id_t FEC_DEC_MSG = 0;
};

typedef monitor_proto<monitor_dec_msg, proto_messages::FEC_DEC_MSG> proto_fec_t;

} // namespace dtl
} // namespace gr

#endif