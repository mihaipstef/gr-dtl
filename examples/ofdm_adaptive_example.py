#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: OFDM Adaptive Example
# GNU Radio version: 3.10.4.0

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from gnuradio import blocks
from gnuradio import channels
from gnuradio.filter import firdes
from gnuradio import dtl
from gnuradio import fec
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation



from gnuradio import qtgui

class ofdm_adaptive_example(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "OFDM Adaptive Example", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("OFDM Adaptive Example")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "ofdm_adaptive_example")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 16000
        self.n_bytes = n_bytes = 100

        ##################################################
        # Blocks
        ##################################################
        self.fec_ber_bf_0 = fec.ber_bf(False, 100, -7.0)
        self.dtl_ofdm_adaptive_tx_config_0 = tx_config = dtl.ofdm_adaptive_tx_config(
            fft_len=64,
            cp_len=16,
            rolloff=0,
            debug=False,
            scramble_bits=False)
        self.dtl_ofdm_adaptive_tx_0 = dtl.ofdm_adaptive_tx(tx_config)
        self.dtl_ofdm_adaptive_rx_config_0 = rx_config = dtl.ofdm_adaptive_rx_config(
            fft_len=64,
            cp_len=16,
            occupied_carriers=(),
            pilot_carriers=(),
            pilot_symbols=(),
            rolloff=0,
            debug=False,
            scramble_bits=False)
        self.dtl_ofdm_adaptive_rx_0 = dtl.ofdm_adaptive_rx(rx_config)
        self.channels_channel_model_0_0 = channels.channel_model(
            noise_voltage=0.0,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0 + 1.0j],
            noise_seed=0,
            block_tags=True)
        self.channels_channel_model_0 = channels.channel_model(
            noise_voltage=0.0,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0 + 1.0j],
            noise_seed=0,
            block_tags=True)
        self.blocks_vector_source_x_0_0 = blocks.vector_source_b(range(10*n_bytes), True, 1, ())
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_char*1, samp_rate,True)
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, n_bytes, 'packet_len')
        self.blocks_message_debug_1 = blocks.message_debug(True)
        self.blocks_message_debug_0 = blocks.message_debug(True)


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.dtl_ofdm_adaptive_rx_0, 'feedback'), (self.blocks_message_debug_0, 'print'))
        self.msg_connect((self.dtl_ofdm_adaptive_tx_0, 'feedback_rcvd'), (self.blocks_message_debug_1, 'print'))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.dtl_ofdm_adaptive_tx_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.fec_ber_bf_0, 0))
        self.connect((self.blocks_vector_source_x_0_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.channels_channel_model_0, 0), (self.dtl_ofdm_adaptive_rx_0, 0))
        self.connect((self.channels_channel_model_0_0, 0), (self.dtl_ofdm_adaptive_tx_0, 1))
        self.connect((self.dtl_ofdm_adaptive_rx_0, 1), (self.channels_channel_model_0_0, 0))
        self.connect((self.dtl_ofdm_adaptive_rx_0, 0), (self.fec_ber_bf_0, 1))
        self.connect((self.dtl_ofdm_adaptive_tx_0, 0), (self.channels_channel_model_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "ofdm_adaptive_example")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

    def get_n_bytes(self):
        return self.n_bytes

    def set_n_bytes(self, n_bytes):
        self.n_bytes = n_bytes
        self.blocks_stream_to_tagged_stream_0.set_packet_len(self.n_bytes)
        self.blocks_stream_to_tagged_stream_0.set_packet_len_pmt(self.n_bytes)
        self.blocks_vector_source_x_0_0.set_data(range(10*self.n_bytes), ())




def main(top_block_cls=ofdm_adaptive_example, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
