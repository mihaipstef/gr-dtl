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
        ofdm_adaptive_config,
        ofdm_adaptive_feedback_decision,
        constellation_type_t,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        ofdm_adaptive_config,
        ofdm_adaptive_feedback_decision,
        constellation_type_t,
    )


class qa_ofdm_adaptive_feedback_decision(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()
        self.decision_counter = 3
        self.mcs = [(snr_th, (cnst, 0)) for (snr_th, (cnst, _)) in ofdm_adaptive_config.ofdm_adaptive_config.mcs]

    def tearDown(self):
        self.tb = None

    def test_feedback_decision(self):
        feedback_decision = ofdm_adaptive_feedback_decision(1, self.decision_counter, self.mcs, 1)

        test_input = ((self.decision_counter + 1) * 3 - 1) * [(constellation_type_t.QPSK, 27),] + (
            (self.decision_counter + 1) * 3 - 1) * [(constellation_type_t.PSK8, 14.5),]
        expected_decision = self.decision_counter * [constellation_type_t.QPSK,] + (self.decision_counter+1) * [
            constellation_type_t.PSK8,] + (self.decision_counter+1) * [constellation_type_t.QAM16,] + self.decision_counter * [
            constellation_type_t.QAM16,] + (self.decision_counter+1) * [constellation_type_t.PSK8,] + (self.decision_counter+1) * [constellation_type_t.QPSK,]

        decision = []
        for (current_cnst, estimated_snr) in test_input:
            decision.append(feedback_decision.get_feedback(estimated_snr))
        self.assertEqual(list(zip(*decision))[0], tuple(expected_decision))


if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_feedback_decision)
