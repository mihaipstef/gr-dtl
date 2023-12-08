/* -*- c++ -*- */
/*
 * Copyright 2023 gr-dtl author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <boost/test/unit_test.hpp>
#include <gnuradio/monitoring/monitor_probe.h>
#include <gnuradio/monitoring/monitor_parser.h>
#include <gnuradio/monitoring/monitor_proto.h>
#include "ofdm_adaptive_monitor.h"
#include "proto/monitor_ofdm.pb.h"

namespace gr {
namespace dtl {


struct test_sender: public message_sender_base {
    uint8_t* raw_msg;
    void send(zmq::message_t* msg) override
    {
        memcpy(raw_msg, msg->data(), msg->size());
    }
    size_t get_msg_counter()
    {
        return 0;
    }
};


BOOST_AUTO_TEST_CASE(monitor_fec_test_any)
{
    //parser_registry::load_parsers();
    std::shared_ptr<test_sender> sender = std::make_shared<test_sender>();
    monitor_probe::sptr probe = monitor_probe::make("test", sender);
    proto_fec_builder_t msg;
    std::vector<uint8_t> sender_buf(msg.size() + 1);
    sender->raw_msg = &sender_buf[0];
    for (int i=0; i<1; i++) {
        pmt::pmt_t blob = msg.build_any(
            std::make_pair("tb_no", 10),
            std::make_pair("frame_payload", 1000),
            std::make_pair("avg_it", 10.0)

        );
        probe->monitor_msg_handler(blob);
        gr::dtl::parse_result result;
        parse(sender->raw_msg, msg.size() + 1, result);

        auto& r = result.dict_msg;
        BOOST_CHECK_GT(r.size(), 0);
        BOOST_CHECK(r.find("time") != r.end());
        BOOST_CHECK_EQUAL(std::get<long>(r["tb_no"]), 10);
        BOOST_CHECK_EQUAL(std::get<long>(r["frame_payload"]), 1000);
        BOOST_CHECK_EQUAL(std::get<long>(r["nmsgs"]), 0);
    }
}

BOOST_AUTO_TEST_CASE(monitor_fec_test_blob)
{
    //parser_registry::load_parsers();
    std::shared_ptr<test_sender> sender = std::make_shared<test_sender>();
    monitor_probe::sptr probe = monitor_probe::make("test", sender);
    proto_fec_builder_t msg;
    std::vector<uint8_t> sender_buf(msg.size() + 1);
    sender->raw_msg = &sender_buf[0];
    for (int i=0; i<1; i++) {


        pmt::pmt_t blob = msg.build(
            std::make_pair("tb_no", 10),
            std::make_pair("frame_payload", 1000)
        );
        probe->monitor_msg_handler(blob);

        gr::dtl::parse_result result;
        parse(sender->raw_msg, msg.size() + 1, result);
        auto& r = result.dict_msg;
        BOOST_CHECK_GT(r.size(), 0);
        BOOST_CHECK(r.find("time") != r.end());
        BOOST_CHECK_EQUAL(std::get<long>(r["tb_no"]), 10);
        BOOST_CHECK_EQUAL(std::get<long>(r["frame_payload"]), 1000);
        BOOST_CHECK_EQUAL(std::get<long>(r["nmsgs"]), 0);
    }
}


} /* namespace dtl */
} /* namespace gr */
