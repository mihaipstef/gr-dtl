#!/usr/bin/env python
#
# Copyright 2013 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
#

from gnuradio import blocks
from gnuradio import channels
from gnuradio import dtl
from gnuradio import gr, gr_unittest
from ofdm_adaptive_rx import ofdm_adaptive_rx
from ofdm_adaptive_tx import ofdm_adaptive_tx
from ofdm_adaptive_config import (
    ofdm_adaptive_tx_config as tx_cfg,
    ofdm_adaptive_rx_config as rx_cfg,
)
import os
import pmt
import random
import sys
import time
import typing as t


def subfinder(l, p):
    n_l, n_p = len(l), len(p)
    for i in range(n_l):
        if l[i:i+n_p] == p:
            return (i, l[i:i+n_p])
    return (0, [])


class qa_ofdm_adaptive(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()
        self.frame_len = 20
        self.data_carriers = len(tx_cfg.occupied_carriers[0])
        self.known_constellations = ((sys.float_info.min, dtl.constellation_type_t.BPSK), (
            15, dtl.constellation_type_t.QPSK), (18, dtl.constellation_type_t.PSK8),)
        test_codes_dir = os.path.dirname(__file__)
        self.codes = (f"{test_codes_dir}/n_0100_k_0023_gap_10.alist",f"{test_codes_dir}/n_0100_k_0027_gap_04.alist")


    def tearDown(self):
        self.tb = None

    def test_001_direct_txrx(self):
        return
        """
        Test Tx/Rx of multiple packets with diferent constellations.
        """
        tx_samples = []

        # Tx
        buffer_size = 32000
        test_data = [random.randint(0, 255) for x in range(buffer_size)]
        src = blocks.vector_source_b(test_data)
        feedback_src = blocks.vector_source_c([0 for _ in range(50)])
        tx = ofdm_adaptive_tx(
            tx_cfg(frame_length=self.frame_len, constellations=self.known_constellations, stop_no_input=True, codes_alist=self.codes))
        sink = blocks.vector_sink_c()
        self.tb.connect(src, (tx, 0), sink)
        self.tb.connect(feedback_src, (tx, 1))

        # self.tb.connect((tx,0), blocks.file_sink(
        #     gr.sizeof_char, "/tmp/tx.dat"))

        cnst = dtl.constellation_type_t.QPSK
        tx.set_constellation(cnst)

        time.sleep(1)

        self.tb.run()

        tx_samples = sink.data()

        print(f"tx samples={len(tx_samples)}")
        tx_samples = [0 for _ in range(100)] + \
            tx_samples + [0 for x in range(2000)]

        # Channel
        freq_offset = 0
        channel = channels.channel_model(
            0, frequency_offset=freq_offset * 1.0/tx_cfg.fft_len,)

        # Rx
        rx_src = blocks.vector_source_c(tx_samples)
        rx = ofdm_adaptive_rx(
            rx_cfg(frame_length=self.frame_len, sync_threshold=0.99, constellations=self.known_constellations, codes_alist=self.codes))
        rx_sink = blocks.vector_sink_b()
        null_sink = blocks.null_sink(gr.sizeof_gr_complex)

        self.tb.connect(rx_src, rx)
        self.tb.connect((rx, 0), rx_sink)
        self.tb.connect((rx, 1), blocks.null_sink(gr.sizeof_gr_complex))

        self.tb.run()
        rx_data = rx_sink.data()
        packet_success = test_data == rx_data[:buffer_size]
        d = [t-r for t, r in zip(test_data, rx_data[:buffer_size])]
        print(f"count correct bytes = {d.count(0)}/{buffer_size}")
        errors = [i for i in range(len(d)) if d[i]]
        if len(errors) > 0:
            print(errors[:20])
            first_err = errors[0]
            print(len(test_data), len(rx_data))
            print(test_data[first_err-20:first_err+20])
            print(rx_data[first_err-20:first_err+20])
            print(subfinder(test_data, rx_data[first_err: first_err+20]))
        assert (packet_success)

    def test_002_feedback_txrx(self):
        return
        test_data = [
            [int(dtl.constellation_type_t.QAM16), 4],
            [int(dtl.constellation_type_t.QPSK), 3],
        ]
        msgs = [pmt.cons(pmt.PMT_NIL, pmt.init_u8vector(2, d))
                for d in test_data]

        rx = ofdm_adaptive_rx(rx_cfg)
        rx_src = blocks.vector_source_c([0 for _ in range(800)])
        feedback_sink = blocks.vector_sink_c()

        self.tb.connect(rx_src, rx)
        self.tb.connect((rx, 0), blocks.null_sink(gr.sizeof_char))
        self.tb.connect((rx, 1), feedback_sink)

        for msg in msgs:
            rx.feedback_formatter._post(pmt.intern("in"), msg)
            self.tb.start()
            # HACK: Sleep to allow the message through
            # Not very robust
            time.sleep(1)
            self.tb.stop()
            self.tb.wait()

        tx_tb = gr.top_block()

        tx = ofdm_adaptive_tx(tx_cfg(codes_alist=self.codes))

        # Channel
        freq_offset = 0
        channel = channels.channel_model(
            0, frequency_offset=freq_offset,)

        feedback_src = blocks.vector_source_c(
            [0 for _ in range(1000)] +  list(feedback_sink.data()) + [0 for _ in range(1000)], repeat=False)
        tx_src = blocks.vector_source_b([0 for _ in range(200000)])
        tx_tb.connect(tx_src, (tx, 0))
        tx_tb.connect(feedback_src, channel, (tx, 1))
        tx_tb.connect(tx, blocks.null_sink(gr.sizeof_gr_complex))
        msg_debug = blocks.message_debug()
        tx_tb.msg_connect(tx, "monitor", msg_debug, "store")
        tx_tb.start()
        # HACK: Sleep to allow the message through
        # Not very robust
        time.sleep(1)
        tx_tb.stop()
        tx_tb.wait()

        expected_messages = {
            "feedback_fec_key": [4, 3],
            "feedback_constellation_key": [4, 2],
            "frame_count_key": list(range(100)),
            "fec_feedback_key": list(range(10))
        }

        for i in range(msg_debug.num_messages()):
            msg = msg_debug.get_message(i)
            if pmt.is_dict(msg):
                items = pmt.dict_items(msg)
                for i in range(pmt.length(items)):
                    item = pmt.nth(i, items)
                    key = pmt.to_python(pmt.car(item))
                    val = pmt.to_python(pmt.cdr(item))
                    self.assertTrue(key in expected_messages)
                    self.assertTrue(val in expected_messages[key])


    def test_003_direct_fec_txrx(self):
        """
        Test Tx/Rx of multiple packets with diferent constellations.
        """
        tx_samples = []

        # Tx
        buffer_size = 3000
        test_data = [random.randint(0, 255) for x in range(buffer_size)]
        src = blocks.vector_source_b(test_data)
        feedback_src = blocks.vector_source_c([0 for _ in range(50)])
        tx = ofdm_adaptive_tx(
            tx_cfg(fec=True, frame_length=self.frame_len, constellations=self.known_constellations, stop_no_input=True, codes_alist=self.codes))
        sink = blocks.vector_sink_c()
        self.tb.connect(src, (tx, 0), sink)
        self.tb.connect(feedback_src, (tx, 1))

        # self.tb.connect((tx,0), blocks.file_sink(
        #     gr.sizeof_char, "/tmp/tx.dat"))

        cnst = dtl.constellation_type_t.QPSK
        tx.set_constellation(cnst)

        time.sleep(1)

        self.tb.run()
        return

        tx_samples = sink.data()

        print(f"tx samples={len(tx_samples)}")
        tx_samples = [0 for _ in range(100)] + \
            tx_samples + [0 for x in range(2000)]

        # Channel
        freq_offset = 0
        channel = channels.channel_model(
            0, frequency_offset=freq_offset * 1.0/tx_cfg.fft_len,)

        # Rx
        rx_src = blocks.vector_source_c(tx_samples)
        rx = ofdm_adaptive_rx(
            rx_cfg(fec=True, frame_length=self.frame_len, sync_threshold=0.99, constellations=self.known_constellations, codes_alist=self.codes))
        rx_sink = blocks.vector_sink_b()
        null_sink = blocks.null_sink(gr.sizeof_gr_complex)

        self.tb.connect(rx_src, rx)
        self.tb.connect((rx, 0), rx_sink)
        self.tb.connect((rx, 1), blocks.null_sink(gr.sizeof_gr_complex))

        self.tb.run()
        return
        rx_data = rx_sink.data()
        packet_success = test_data == rx_data[:buffer_size]
        d = [t-r for t, r in zip(test_data, rx_data[:buffer_size])]
        print(f"count correct bytes = {d.count(0)}/{buffer_size}")
        errors = [i for i in range(len(d)) if d[i]]
        if len(errors) > 0:
            print(errors[:20])
            first_err = errors[0]
            print(len(test_data), len(rx_data))
            print(test_data[first_err-20:first_err+20])
            print(rx_data[first_err-20:first_err+20])
            print(subfinder(test_data, rx_data[first_err: first_err+20]))
        assert (packet_success)



if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive)
