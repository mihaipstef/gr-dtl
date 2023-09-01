#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import (
    blocks,
    channels,
    digital,
    gr,
    gr_unittest,
)
import numpy
import pmt

# from gnuradio import blocks
try:
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        estimated_snr_tag_key,
        ofdm_adaptive_config,
        ofdm_adaptive_frame_equalizer_vcvc,
        ofdm_adaptive_payload_equalizer,
        ofdm_adaptive_frame_snr_simple,
        ofdm_adaptive_feedback_decision,
        noise_tag_key,
    )
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        constellation_type_t,
        get_constellation_tag_key,
        estimated_snr_tag_key,
        ofdm_adaptive_config,
        ofdm_adaptive_frame_equalizer_vcvc,
        ofdm_adaptive_payload_equalizer,
        ofdm_adaptive_frame_snr_simple,
        ofdm_adaptive_feedback_decision,
        noise_tag_key,
    )


class qa_ofdm_adaptive_frame_equalizer_vcvc(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_equalizer(self):
        consts = {
            constellation_type_t.QPSK: digital.constellation_qpsk(),
            constellation_type_t.PSK8: digital.constellation_8psk(),
        }
        fft_len = 8

        for i, (c, cnst) in enumerate(consts.items()):
            #            4   5  6  7   0  1  2   3
            tx_data = [-1, -1, 1, 2, -1, 3, 0, -1,  # 0
                       -1, -1, 0, 2, -1, 2, 0, -1,  # 8
                       -1, -1, 3, 0, -1, 1, 0, -1,  # 16 (Pilot symbols)
                       -1, -1, 1, 1, -1, 0, 2, -1]  # 24
            tx_signal = [
                cnst.map_to_points_v(x)[0] if x != -
                1 else 0 for x in tx_data]
            occupied_carriers = ((1, 2, 6, 7),)
            pilot_carriers = ((), (), (1, 2, 6, 7), ())
            pilot_symbols = (
                [], [], [cnst.map_to_points_v(x)[0] for x in (1, 0, 3, 0)], []
            )
            equalizer = ofdm_adaptive_payload_equalizer(
                fft_len,
                [k for k in consts],
                ofdm_adaptive_frame_snr_simple(float(0.1)),
                occupied_carriers,
                pilot_carriers,
                pilot_symbols,
                0,
                0.01)
            channel = [
                0, 0, 1, 1, 0, 1, 1, 0,
                0, 0, 1, 1, 0, 1, 1, 0,
                0, 0, 1j, 1j, 0, 1j, 1j, 0,
                0, 0, 1j, 1j, 0, 1j, 1j, 0
            ]
            for idx in range(fft_len, 2 * fft_len):
                channel[idx] = channel[idx - fft_len] * \
                    numpy.exp(1j * .1 * numpy.pi * (numpy.random.rand() - .5))
                idx2 = idx + 2 * fft_len
                channel[idx2] = channel[idx2] * \
                    numpy.exp(1j * 0 * numpy.pi * (numpy.random.rand() - .5))
            chan_tag = gr.tag_t()
            chan_tag.offset = 0
            chan_tag.key = pmt.string_to_symbol("ofdm_sync_chan_taps")
            chan_tag.value = pmt.init_c32vector(fft_len, channel[:fft_len])
            const_tag = gr.tag_t()
            const_tag.offset = 0
            const_tag.key = get_constellation_tag_key()
            const_tag.value = pmt.from_long(c)
            rx_signal = numpy.multiply(tx_signal, channel) + [x if y != 0 else 0 for (
                x, y) in zip(numpy.random.normal(0, 0.001, len(tx_signal)), tx_signal)]

            src = blocks.vector_source_c(
                rx_signal, False, fft_len, (chan_tag, const_tag))

            feedback_decision_sink = blocks.message_debug()

            eq = ofdm_adaptive_frame_equalizer_vcvc(
                equalizer.base(), ofdm_adaptive_feedback_decision(
                    1, 3, [(snr_th, (cnst, 0)) for (snr_th, (cnst, _)) in ofdm_adaptive_config.ofdm_adaptive_config.mcs]), 0, "tsb_key", "frame_no_key", True, True)
            self.tb.msg_connect(eq, 'feedback_port',
                                feedback_decision_sink, 'store')
            sink = blocks.tsb_vector_sink_c(fft_len, tsb_key="tsb_key")
            stream_to_tagged = blocks.stream_to_tagged_stream(
                gr.sizeof_gr_complex, fft_len, len(tx_data) // fft_len, "tsb_key")
            tag_sink = blocks.tag_debug(fft_len * gr.sizeof_gr_complex, "tags")
            self.tb.connect(stream_to_tagged, tag_sink)
            self.tb.connect(
                src,
                stream_to_tagged,
                eq,
                sink
            )

            self.tb.run()

            out_syms = numpy.array(sink.data()[0])

            def demod(syms):
                return [
                    cnst.decision_maker_v(
                        (x,)) if x != 0 else -1 for x in syms]
            rx_data = demod(out_syms)

            self.assertEqual(tx_data, rx_data)
            print([pmt.symbol_to_string(t.key) for t in sink.tags()])
            self.assertEqual(len(sink.tags()), 6)
            self.assertIn(get_constellation_tag_key(),
                          [t.key for t in sink.tags()])
            self.assertIn(estimated_snr_tag_key(), [
                          t.key for t in sink.tags()])
            self.assertIn(noise_tag_key(), [
                          t.key for t in sink.tags()])
            self.assertEqual(feedback_decision_sink.num_messages(), 1)
            feedback_msg = feedback_decision_sink.get_message(0)
            self.assertEqual(pmt.u8vector_elements(
                pmt.cdr(feedback_msg)), [1, 0])


if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_frame_equalizer_vcvc)
