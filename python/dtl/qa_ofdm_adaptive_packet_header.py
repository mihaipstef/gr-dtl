#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import (
    blocks,
    digital,
    gr_unittest,
)
from gnuradio.gr import (
    packet_utils,
    tag_t,
    top_block,
)
import numpy
import pmt

# from gnuradio import blocks
try:
    from gnuradio.dtl import ofdm_adaptive_packet_header
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import ofdm_adaptive_packet_header


class qa_ofdm_adaptive_packet_header(gr_unittest.TestCase):

    def setUp(self):
        self.tb = top_block()
        self._occupied_carriers_real = (list(range(-26, -21)) + list(range(-20, -7)) + list(
            range(-6, 0)) + list(range(1, 7)) + list(range(8, 21)) + list(range(22, 27)),)
        self._occupied_carriers_dummy_32 = (list(range(32)),)
        self._occupied_carriers_dummy_40 = (list(range(40)),)

    def tearDown(self):
        self.tb = None

    def test_pass_constellation_through_header(self):
        packets = ((1, 2, 3, 4), (1, 2), (1, 2, 3, 4))

        data, tags = packet_utils.packets_to_vectors(
            packets, "len_key"
        )
        offset = 0

        # Add frame constellation tag to each packet
        for p in packets:
            tag = tag_t()
            tag.offset = offset
            tag.key = pmt.string_to_symbol("frame_constellation")
            tag.value = pmt.from_long(5)
            tags.append(tag)
            offset = offset + len(p)

        src = blocks.vector_source_b(data, tags=tags)
        formatter = ofdm_adaptive_packet_header(
            self._occupied_carriers_dummy_40, 1, "len_key", "frame_len_key", "head_num", 1, 1, False)
        self.assertEqual(formatter.header_len(), 40)
        self.assertEqual(
            pmt.symbol_to_string(
                formatter.len_tag_key()),
            "len_key")
        header_gen = digital.packet_headergenerator_bb(
            formatter.formatter(), "len_key")
        sink_format = blocks.vector_sink_b()
        self.tb.connect(src, header_gen, sink_format)

        # Connect parser to test
        header_parser = digital.packet_headerparser_b(
            formatter.formatter())
        sink_parse = blocks.message_debug()
        self.tb.connect(header_gen, header_parser)
        self.tb.msg_connect(header_parser, "header_data", sink_parse, "store")

        self.tb.run()

        # 0x04 0x00 0x00 0x00 0x05 0xad
        # 0x02 0x00 0x01 0x00 0x05 0x8d
        # 0x04 0x00 0x02 0x00 0x05 0x7b
        expected_format_data = [
            #                                  |                                   |                       |
            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1,
            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 
        ]
        self.assertEqual(sink_format.data(), expected_format_data)
        for i in range(len(packets)):
            msg = pmt.to_python(sink_parse.get_message(i))
            self.assertEqual(
                msg, {
                    "len_key": len(packets[i]), "head_num": i, "frame_constellation": 5, "frame_len_key": 8})

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_packet_header)
