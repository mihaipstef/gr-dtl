/* -*- c++ -*- */
/*
 * Copyright 2023 gr-dtl author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <boost/test/unit_test.hpp>
#include <gnuradio/dtl/monitor_probe.h>
#include <gnuradio/dtl/monitor_probe_parser.h>
#include <gnuradio/dtl/monitor_proto.h>
#include "proto/monitor_fec.pb.h"

namespace gr {
namespace dtl {


struct test_sender: public message_sender_base {
    uint8_t* raw_msg;
    void send(zmq::message_t* msg) override
    {
        memcpy(raw_msg, msg->data(), msg->size());
    }
};


BOOST_AUTO_TEST_CASE(monitor_fec_test_any)
{
    std::shared_ptr<test_sender> sender = std::make_shared<test_sender>();
    monitor_probe::sptr probe = monitor_probe::make("test", sender);
    monitor_proto<monitor_dec_msg> msg;
    std::vector<uint8_t> sender_buf(msg.size() + 1);
    sender->raw_msg = &sender_buf[0];

    for (int i=0; i<100; i++) {
        pmt::pmt_t blob = msg.build_any(
            std::make_pair("tb_no", 10),
            std::make_pair("frame_payload", 1000)
        );
        probe->monitor_msg_handler(blob);

        auto result = parse_monitor_msg<monitor_dec_msg>(sender->raw_msg, msg.size() + 1);

        BOOST_CHECK_NE(result.second, nullptr);
        BOOST_CHECK_NE(result.first, nullptr);
        BOOST_CHECK_EQUAL(result.second->tb_no(), 10);
        BOOST_CHECK_EQUAL(result.second->frame_payload(), 1000);
        BOOST_CHECK_EQUAL(result.first->nmsgs(), 0);
    }
}

BOOST_AUTO_TEST_CASE(monitor_fec_test_blob)
{
    std::shared_ptr<test_sender> sender = std::make_shared<test_sender>();
    monitor_probe::sptr probe = monitor_probe::make("test", sender);
    monitor_proto<monitor_dec_msg> msg;
    std::vector<uint8_t> sender_buf(msg.size() + 1);
    sender->raw_msg = &sender_buf[0];

    for (int i=0; i<100; i++) {


        pmt::pmt_t blob = msg.build(
            std::make_pair("tb_no", 10),
            std::make_pair("frame_payload", 1000)
        );
        probe->monitor_msg_handler(blob);

        auto result = parse_monitor_msg<monitor_dec_msg>(sender->raw_msg, msg.size() + 1);

        BOOST_CHECK_NE(result.second, nullptr);
        BOOST_CHECK_NE(result.first, nullptr);
        BOOST_CHECK_EQUAL(result.second->tb_no(), 10);
        BOOST_CHECK_EQUAL(result.second->frame_payload(), 1000);
        BOOST_CHECK_EQUAL(result.first->nmsgs(), 0);

    }
}


} /* namespace dtl */
} /* namespace gr */
