#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2023 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import (
    blocks,
    digital,
    gr,
    gr_unittest,
)
import pmt
import time
# from gnuradio import blocks
try:
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        ofdm_adaptive_config,
        ofdm_adaptive_constellation_metric_vcvf,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        ofdm_adaptive_config,
        ofdm_adaptive_constellation_metric_vcvf,
    )


class qa_ofdm_adaptive_constellation_metric_vcvf(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()
        self.fft_len = 8
        self.cnst_psk8 = digital.constellation_8psk()
        self.cnst_bpsk = digital.constellation_bpsk()

    def tearDown(self):
        self.tb = None

    def test_001_simple(self):

        data = [0 for _ in range(self.fft_len*2)]
        psk8_signal = [self.cnst_psk8.map_to_points_v(x)[0] for x in data]
        bpsk_signal = [self.cnst_bpsk.map_to_points_v(x)[0] for x in data]

        cnst_tag0 = gr.tag_t()
        cnst_tag0.key = get_constellation_tag_key()
        cnst_tag0.offset = 0
        cnst_tag0.value = pmt.from_long(constellation_type_t.PSK8)
        cnst_tag1 = gr.tag_t()
        cnst_tag1.key = get_constellation_tag_key()
        cnst_tag1.offset = 8
        cnst_tag1.value = pmt.from_long(constellation_type_t.PSK8)

        psk8_src = blocks.vector_source_c(
            psk8_signal, False, self.fft_len, (cnst_tag0, cnst_tag1))
        bpsk_src = blocks.vector_source_c(
            bpsk_signal, False, self.fft_len, (cnst_tag0, cnst_tag1))
        qpsk_stream_to_tagged = blocks.stream_to_tagged_stream(
                gr.sizeof_gr_complex, self.fft_len, len(data) // self.fft_len, "length")
        bpsk_stream_to_tagged = blocks.stream_to_tagged_stream(
                gr.sizeof_gr_complex, self.fft_len, len(data) // self.fft_len, "length")
        metrics = ofdm_adaptive_constellation_metric_vcvf(self.fft_len, list(range(
            self.fft_len)), list(zip(*ofdm_adaptive_config.ofdm_adaptive_config.constellations))[1], "length")

        sink = blocks.tsb_vector_sink_f(self.fft_len, tsb_key="length")

        self.tb.connect(psk8_src, qpsk_stream_to_tagged, (metrics, 0))
        self.tb.connect(bpsk_src, bpsk_stream_to_tagged, (metrics, 1))
        self.tb.connect(metrics, sink)
        self.tb.start()
        time.sleep(1)
        self.tb.stop()

        result = sink.data()
        self.assertEqual(self.fft_len, len(result[0]))
        self.assertAlmostEqual(result[0][0], 6.568538665771484)
        self.assertTrue(all(result[0][0] == x for x in result[0]))

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_constellation_metric_vcvf)
