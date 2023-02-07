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
        ofdm_adaptive_feedback_decision,
        constellation_type_t,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        ofdm_adaptive_feedback_decision,
        constellation_type_t,
    )


def print_pmt_dict_keys(d):
    items = pmt.dict_items(d)
    nitems = pmt.length(items)
    for i in range(nitems):
        item = pmt.nth(i, items)
        key = pmt.symbol_to_string(pmt.car(item))
        val = pmt.cdr(item)
        print("{0}: {1}".format(key, val))


class qa_ofdm_adaptive_feedback_decision(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()
        self.decision_counter = 3

    def tearDown(self):
        self.tb = None

    def test_feedback_decision(self):
        feedback_decision = ofdm_adaptive_feedback_decision(1, self.decision_counter)

        test_input = ((self.decision_counter + 1) * 3 - 1) * [(constellation_type_t.QPSK, 23),] + (
            (self.decision_counter + 1) * 3 - 1) * [(constellation_type_t.PSK8, 14.5),]
        expected_decision = self.decision_counter * [constellation_type_t.QPSK,] + (self.decision_counter+1) * [
            constellation_type_t.PSK8,] + (self.decision_counter+1) * [constellation_type_t.QAM16,] + self.decision_counter * [
            constellation_type_t.QAM16,] + (self.decision_counter+1) * [constellation_type_t.PSK8,] + (self.decision_counter+1) * [constellation_type_t.QPSK,]

        decision = []
        for (current_cnst, estimated_snr) in test_input:
            decision.append(feedback_decision.get_feedback(current_cnst, estimated_snr))
        self.assertEqual(decision, expected_decision)


if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_feedback_decision)
