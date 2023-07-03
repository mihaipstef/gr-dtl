#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2023 DTL.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import (
    gr,
    gr_unittest,
    blocks,
)
import os
import pmt
import random
# from gnuradio import blocks
try:
  from gnuradio.dtl import (
    constellation_type_t,
    fec_feedback_key,
    feedback_constellation_key,
    get_bits_per_symbol,
    ofdm_adaptive_fec_frame_bvb,
    ofdm_adaptive_frame_to_stream_vbb,
    ofdm_adaptive_constellation_soft_cf,
    ofdm_adaptive_chunks_to_symbols_bc,
    ofdm_adaptive_fec_decoder,
    make_ldpc_encoders,
    make_ldpc_decoders,
  )
except ImportError:
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.dtl import (
        constellation_type_t,
        fec_feedback_key,
        feedback_constellation_key,
        get_bits_per_symbol,
        ofdm_adaptive_fec_frame_bvb,
        ofdm_adaptive_frame_to_stream_vbb,
        ofdm_adaptive_constellation_soft_cf,
        ofdm_adaptive_chunks_to_symbols_bc,
        ofdm_adaptive_fec_decoder,
        make_ldpc_encoders,
        make_ldpc_decoders,
  )


class qa_ofdm_adaptive_fec_encoder(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block(catch_exceptions=True)
        self.test_codes_dir = os.path.dirname(__file__)
        self.len_key = "len_key"
        self.fec_idx = 1
        self.constellations = [constellation_type_t.BPSK, constellation_type_t.QPSK, constellation_type_t.PSK8, constellation_type_t.QAM16]
        self.max_bps = 4
        codes = [f"{self.test_codes_dir}/n_0100_k_0023_gap_10.alist",f"{self.test_codes_dir}/n_0100_k_0027_gap_04.alist"]
        self.ldpc_encs = make_ldpc_encoders(codes)
        self.ldpc_decs = make_ldpc_decoders(codes)


    def tearDown(self):
        self.tb = None


    def run_flow(self, cnst, fec, frame_len, ofdm_sym_capacity):

        self.frame_len = frame_len
        self.ofdm_sym_capacity = ofdm_sym_capacity

        data = [random.getrandbits(1) for _ in range(int(self.ldpc_encs[fec].get_k() * 69.69))]
        #data = [1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1]

        src = blocks.vector_source_b(
                data * 1)

        feedback = pmt.make_dict()
        feedback = pmt.dict_add(feedback, fec_feedback_key(), pmt.from_long(fec))
        feedback = pmt.dict_add(feedback, feedback_constellation_key(), pmt.from_long(int(cnst)))


        enc = ofdm_adaptive_fec_frame_bvb(
            self.ldpc_encs,
            self.frame_len * self.ofdm_sym_capacity,
            self.max_bps,
            self.len_key
        )
        enc.process_feedback(feedback)

        to_stream = ofdm_adaptive_frame_to_stream_vbb(self.frame_len * self.ofdm_sym_capacity, self.len_key)
        mod = ofdm_adaptive_chunks_to_symbols_bc(self.constellations, self.len_key)

        cnst_dec = ofdm_adaptive_constellation_soft_cf(self.constellations, self.len_key)

        dec = ofdm_adaptive_fec_decoder(
            self.ldpc_decs,
            self.frame_len * self.ofdm_sym_capacity,
            self.max_bps,
            self.len_key
        )

        sink_b = blocks.vector_sink_b()
        sink_vb = blocks.vector_sink_b(self.frame_len * self.ofdm_sym_capacity)


        sink_f = blocks.vector_sink_f()
        sink_c = blocks.vector_sink_c()
        sink_b_dec = blocks.vector_sink_b()

        self.tb.connect(to_stream, sink_b)
        self.tb.connect(mod, sink_c)
        self.tb.connect(cnst_dec, sink_f)
        self.tb.connect(enc, sink_vb)

        self.tb.connect(
            src,
            enc,
            to_stream,
            mod,
            cnst_dec,
            dec,
            sink_b_dec
        )

        self.tb.run()
        # check data
        print(data)
        print(sink_b_dec.data())

        print(len(sink_b_dec.data()), len(data))
        assert len(data) == len(sink_b_dec.data())
        assert data == sink_b_dec.data()

    def test_001_encode_qam16(self):
        #return
        self.run_flow(cnst = constellation_type_t.QAM16, fec=1, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QAM16, fec=2, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QAM16, fec=1, frame_len=10, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QAM16, fec=2, frame_len=10, ofdm_sym_capacity=48)

        # pass

    def test_002_encode_psk8(self):
        #return
        self.run_flow(cnst = constellation_type_t.PSK8, fec=1, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.PSK8, fec=2, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.PSK8, fec=1, frame_len=10, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.PSK8, fec=2, frame_len=10, ofdm_sym_capacity=48)

    def test_003_encode_qpsk(self):
        #return
        self.run_flow(cnst = constellation_type_t.QPSK, fec=1, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QPSK, fec=2, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QPSK, fec=1, frame_len=10, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.QPSK, fec=2, frame_len=10, ofdm_sym_capacity=48)

    def test_004_encode_bpsk(self):
        #return
        self.run_flow(cnst = constellation_type_t.BPSK, fec=1, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.BPSK, fec=2, frame_len=3, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.BPSK, fec=1, frame_len=10, ofdm_sym_capacity=48)
        self.run_flow(cnst = constellation_type_t.BPSK, fec=2, frame_len=10, ofdm_sym_capacity=48)

if __name__ == '__main__':
    gr_unittest.run(qa_ofdm_adaptive_fec_encoder)
