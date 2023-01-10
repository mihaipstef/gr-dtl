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
    sizeof_gr_complex,
    tag_t,
    top_block,
)
import numpy
import pmt

# from gnuradio import blocks
try:
    from gnuradio.dtl import (
        ofdm_adaptive_packet_header,
        get_constellation_tag_key,
        payload_length_key,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        ofdm_adaptive_packet_header,
        get_constellation_tag_key,
        payload_length_key,
    )


class qa_ofdm_adaptive_packet_header(gr_unittest.TestCase):

    def setup_data_tags(self, data):
        return packet_utils.packets_to_vectors(
            data,
            "self.tsb_key"
        )

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
        constellations = ((4, 4), (3, 3), (2, 2))
        packet_lenghts_in_symbols = []

        data, tags = packet_utils.packets_to_vectors(
            packets, "len_key"
        )
        offset = 0
        for t in tags:
            print(t.key, t.value, t.offset)
        # Add frame constellation tag to each packet
        for p, c in zip(packets, constellations):
            tag = tag_t()
            tag.offset = offset
            tag.key = get_constellation_tag_key()
            tag.value = pmt.from_long(c[0])
            tags.append(tag)
            tag = tag_t()
            tag.offset = offset
            tag.key = payload_length_key()
            tag.value = pmt.from_long(len(p))
            tags.append(tag)
            offset = offset + len(p)
            packet_lenghts_in_symbols.append(len(p) * 8 // c[1] + int(len(p) * 8 % c[1] > 0))
        for t in tags:
            print(t.key, t.value, t.offset)
        src = blocks.vector_source_b(data, tags=tags)
        formatter = ofdm_adaptive_packet_header(
            self._occupied_carriers_dummy_40, 1, "len_key", "frame_len_key", "head_num", 1, False)
        self.assertEqual(formatter.header_len(), 40)
        self.assertEqual(
            pmt.symbol_to_string(
                formatter.len_tag_key()),
            "len_key")
        header_gen = digital.packet_headergenerator_bb(
            formatter.formatter(), "len_key")
        sink_format = blocks.vector_sink_b()
        self.tb.connect(src, header_gen, sink_format)

        stop_tags_gate = blocks.tag_gate(1, False)
        self.tb.connect(header_gen, stop_tags_gate)

        # Connect parser to test
        tag_sink = blocks.tag_debug(1, "len_key")
        self.tb.connect(src, tag_sink)
        header_parser = digital.packet_headerparser_b(
            formatter.formatter())
        sink_parse = blocks.message_debug()
        self.tb.connect(stop_tags_gate, header_parser)
        self.tb.msg_connect(header_parser, "header_data", sink_parse, "store")

        self.tb.run()

        print("tags")
        for t in tag_sink.current_tags():
            print(t.key, t.value)

        # 0x04 0x00 0x00 0x00 0x04 0xaa
        # 0x02 0x00 0x01 0x00 0x03 0x9f
        # 0x04 0x00 0x02 0x00 0x02 0x6e
        expected_format_data = [
            #                                  |                                   |                       |
            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1,
            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0,
        ]
        self.assertEqual(sink_format.data(), expected_format_data)
        for i in range(len(packets)):
            msg = pmt.to_python(sink_parse.get_message(i))
            print(msg)
            self.assertEqual(
                msg, {
                    "len_key": packet_lenghts_in_symbols[i],
                    "head_num": i,
                    pmt.symbol_to_string(get_constellation_tag_key()): constellations[i][0 ],
                    "frame_len_key": 1
                }
            )

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_packet_header)
