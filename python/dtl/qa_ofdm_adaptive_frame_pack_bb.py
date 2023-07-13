#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr, gr_unittest
from gnuradio import blocks
import pmt
try:
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        ofdm_adaptive_frame_pack_bb,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        ofdm_adaptive_frame_pack_bb,
    )


class qa_ofdm_adaptive_frame_pack_bb(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_tx_lsb_first(self):
        src_data_2packets = [0b101,] + [0b111,] * 4 + [0b001,] + 11 * [0b000] +\
                                  [0b01,] + [0b11] * 7 + 16 * [0b00]
        expected_data_2packets = [0b11111101, 0b11111111,
                             0b11111101, 0b11111111, ] 
        cnsts = [(0, constellation_type_t.PSK8, 6 + 11), (6 + 11, constellation_type_t.QPSK, 8 + 16)]
        tags = []
        print(src_data_2packets)
        for i, cnst, len in cnsts:
            cnst_tag = gr.tag_t()
            cnst_tag.offset = i
            cnst_tag.key = get_constellation_tag_key()
            cnst_tag.value = pmt.from_long(cnst)
            len_tag = gr.tag_t()
            len_tag.offset = i
            len_tag.key = pmt.string_to_symbol("len_tag")
            len_tag.value = pmt.from_long(len)
            tags += [cnst_tag, len_tag]

        src = blocks.vector_source_b(src_data_2packets, False, 1, tags)
        src_sink = blocks.vector_sink_b()
        self.tb.connect(src, src_sink)
        repack = ofdm_adaptive_frame_pack_bb("len_tag", "", "")
        sink = blocks.vector_sink_b()
        self.tb.connect(src, repack, sink)
        self.tb.run()
        print(src_sink.data())

        self.assertEqual(sink.data(), expected_data_2packets)


if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_frame_pack_bb)
