#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr, gr_unittest, blocks, digital
import pmt

try:
  from gnuradio.dtl import (
    ofdm_adaptive_chunks_to_symbols_bc,
    constellation_type_t,
    ofdm_adaptive_constellation_decoder_cb,
)
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        ofdm_adaptive_chunks_to_symbols_bc,
        constellation_type_t,
        ofdm_adaptive_constellation_decoder_cb,
    )

class qa_ofdm_adaptive_chunks_to_symbols_bc(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_bc_001(self):
        const = [1 + 0j, 0 + 1j,
                 -1 + 0j, 0 - 1j]
        src_data = (0, 1, 2, 3, 3, 2, 1, 0)
        expected_result = [1 + 0j, 0 + 1j, -1 + 0j, 0 - 1j,
                           0 - 1j, -1 + 0j, 0 + 1j, 1 + 0j]

        cnst_tag = gr.tag_t()
        cnst_tag.offset = 0
        cnst_tag.key = pmt.string_to_symbol("frame_constellation")
        cnst_tag.value = pmt.from_long(constellation_type_t.QPSK)
        len_tag = gr.tag_t()
        len_tag.offset = 0
        len_tag.key = pmt.string_to_symbol("len_tag")
        len_tag.value = pmt.from_long(8)
        tags = [cnst_tag, len_tag]

        src = blocks.vector_source_b(src_data, False, 1, tags)
        mod = ofdm_adaptive_chunks_to_symbols_bc([constellation_type_t.QPSK], "len_tag")
        demod = ofdm_adaptive_constellation_decoder_cb([constellation_type_t.QPSK], "len_tag")

        dst = blocks.vector_sink_c()
        self.tb.connect(src, mod)
        self.tb.connect(mod, demod)
        self.tb.connect(demod, dst)
        self.tb.run()

        actual_result = dst.data()
        self.assertEqual(src_data, actual_result)

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_chunks_to_symbols_bc)
