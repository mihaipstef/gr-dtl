/* -*- c++ -*- */
/*
 * Copyright 2023 gr-dtl author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <boost/test/unit_test.hpp>
#include "proto/monitor_fec.pb.h"
#include "monitor_proto.h"

namespace gr {
namespace dtl {

BOOST_AUTO_TEST_CASE(monitor_fec_test)
{
    monitor_proto<monitor_dec_msg> msg;
    pmt::pmt_t blob = msg.build(
        std::make_pair("tb_no", 10),
        std::make_pair("frame_payload", 1000)
    );
    std::stringbuf buf;
    pmt::serialize(blob, buf);

    pmt::pmt_t d_blob = pmt::deserialize(buf);
    monitor_proto<monitor_dec_msg> d_msg;
    auto result = d_msg.parse(d_blob);
    BOOST_CHECK_NE(result.second, nullptr);
    BOOST_CHECK_NE(result.first, 0);
    BOOST_CHECK_EQUAL(result.second->tb_no(), 10);
    BOOST_CHECK_EQUAL(result.second->frame_payload(), 1000);
}

} /* namespace dtl */
} /* namespace gr */
