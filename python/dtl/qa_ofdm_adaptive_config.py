#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

try:
    from gnuradio.dtl import (
        ofdm_adaptive_rx_config,
        ofdm_adaptive_tx_config,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        ofdm_adaptive_rx_config,
        ofdm_adaptive_tx_config,
    )
import unittest

class qa_ofdm_adaptive_config(unittest.TestCase):

    def test_tx_config_defaults(self):
        cfg = ofdm_adaptive_tx_config()
        self.assertEqual(cfg.fft_len, 64)
        cfg = ofdm_adaptive_tx_config(fft_len=16)
        self.assertEqual(cfg.fft_len, 16)

    def test_rx_config_defaults(self):
        cfg = ofdm_adaptive_rx_config()
        self.assertEqual(cfg.fft_len, 64)
        cfg = ofdm_adaptive_rx_config(fft_len=16)
        self.assertEqual(cfg.fft_len, 16)

if __name__ == '__main__':
    unittest.main()
