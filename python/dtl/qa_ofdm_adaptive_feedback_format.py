#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import (
    blocks,
    gr,
    gr_unittest,
    digital,
)
import pmt
import sys
import time

try:
    from gnuradio.dtl import (
        feedback_constellation_key,
        fec_feedback_key,
        ofdm_adaptive_feedback_format,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        feedback_constellation_key,
        fec_feedback_key,
        ofdm_adaptive_feedback_format,
    )

def print_pmt_dict_keys(d):
    items = pmt.dict_items(d)
    nitems = pmt.length(items)
    for i in range(nitems):
        item = pmt.nth(i, items)
        key = pmt.symbol_to_string(pmt.car(item))
        val = pmt.cdr(item)
        print("{0}: {1}".format(key, val))

class qa_ofdm_adaptive_feedback_format(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_async_formatter(self):
        access_code = digital.packet_utils.default_access_code
        constellation_type = 2
        fec_secheme = 6

        header_format = ofdm_adaptive_feedback_format(access_code, 0)

        formatter = digital.protocol_formatter_async(header_format)
        sink_header = blocks.message_debug()
        sink_payload = blocks.message_debug()

        self.tb.msg_connect(formatter, 'header', sink_header, 'store')
        self.tb.msg_connect(formatter, 'payload', sink_payload, 'store')

        send_feedback = pmt.init_u8vector(2, [constellation_type, fec_secheme])
        msg = pmt.cons(pmt.PMT_NIL, send_feedback)

        # Pass the meeasge to the formatter
        port = pmt.intern("in")
        formatter.to_basic_block()._post(port, msg)

        self.tb.start()

        while (sink_header.num_messages() < 1):
            time.sleep(0.1)

        self.tb.stop()
        self.tb.wait()

        result_header = pmt.cdr(sink_header.get_message(0))
        header = bytes(pmt.u8vector_elements(result_header))

        rx_access_code = header[0:len(access_code)//8]
        rx_constellation_type = header[len(access_code)//8:1+len(access_code)//8]
        rx_fec_secheme = header[1+len(access_code)//8:2+len(access_code)//8]

        self.assertEqual(int(access_code,2 ).to_bytes(len(access_code)//8, "big"), rx_access_code)
        self.assertEqual(rx_constellation_type, constellation_type.to_bytes(1, "big"))
        self.assertEqual(rx_fec_secheme, fec_secheme.to_bytes(1, "big"))

        header_to_parse = header[0:3+len(access_code)//8]
        header_to_parse = "".join(f'{x:08b}' for x in header_to_parse)
        header_to_parse = [int(c) for c in header_to_parse]

        src_header = blocks.vector_source_b(header_to_parse)

        parser = digital.protocol_parser_b(header_format)

        sink_parsed_header = blocks.message_debug()

        self.tb.connect(src_header, parser)

        self.tb.msg_connect(parser, 'info', sink_parsed_header, 'store')

        self.tb.start()
        while sink_parsed_header.num_messages() < 1:
            time.sleep(0.1)
        self.tb.stop()
        self.tb.wait()

        parsed_header = sink_parsed_header.get_message(0)

        self.assertTrue(pmt.is_dict(parsed_header))

        self.assertTrue(pmt.dict_has_key(
            parsed_header, feedback_constellation_key()))
        self.assertEqual(pmt.to_long(pmt.dict_ref(
            parsed_header, feedback_constellation_key(), pmt.PMT_F)), constellation_type)
        self.assertTrue(pmt.dict_has_key(
            parsed_header, fec_feedback_key()))
        self.assertEqual(pmt.to_long(pmt.dict_ref(
            parsed_header, fec_feedback_key(), pmt.PMT_F)), fec_secheme)

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_feedback_format)
