from gnuradio import (
    gr,
    blocks,
    digital,
    dtl,
    fft,
    filter,
    pdu,
)
import pmt


class ofdm_adaptive_feedback_rx(gr.hier_block2):
    """Adaptive OFDM feedback RX
    """

    def __init__(self, threshold):
        gr.hier_block2.__init__(self, "ofdm_adaptive_feedback_rx",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(0, 0, 0))

        self.message_port_register_hier_out("feedback_out")

        self.sps = 2  # samples per symbol
        self.eb = 0.35  # excess bw
        self.nfilts = 32
        self.taps = filter.firdes.root_raised_cosine(self.nfilts, self.nfilts,
                                                     1.0, self.eb, 11*self.sps*self.nfilts)
        self.format = dtl.ofdm_adaptive_feedback_format(
            digital.packet_utils.default_access_code, threshold)
        self.constellation = digital.constellation_calcdist(digital.psk_2()[0], digital.psk_2()[1],
                                                            2, 1, digital.constellation.AMPLITUDE_NORMALIZATION).base()
        self.rxmod = digital.generic_mod(
            self.constellation, False, self.sps, True, self.eb, False, False)
        # digital.packet_utils.default_access_code bytes
        self.preamble = [0xac, 0xdd, 0xa4, 0xe2, 0xf2, 0x8c, 0x20, 0xfc]
        self.mark_delays = [0, 0, 34, 56, 87, 119]
        self.modulated_sync_word = digital.modulate_vector_bc(
            self.rxmod.to_basic_block(), self.preamble, [1])
        self.mark_delay = self.mark_delays[self.sps]

        self.parser = digital.protocol_parser_b(self.format)

        self.clock_sync = digital.pfb_clock_sync_ccf(
            self.sps, 6.28/400.0, self.taps, self.nfilts, float(self.nfilts/2), 1.5, 1)

        self.costas_loop = digital.costas_loop_cc(
            (6.28/200.0), self.constellation.arity(), False)
        self.sync_word_correlator = digital.corr_est_cc(
            self.modulated_sync_word, self.sps, self.mark_delay, 0.99, digital.THRESHOLD_ABSOLUTE)
        self.constellation_decoder = digital.constellation_decoder_cb(
            self.constellation)
        self.amp_est_multiplier = blocks.multiply_by_tag_value_cc("amp_est", 1)

        self.connect((self, 0), (self.sync_word_correlator, 0))
        self.connect((self.sync_word_correlator, 0),
                     (self.amp_est_multiplier, 0))
        self.connect((self.amp_est_multiplier, 0), (self.clock_sync, 0))
        self.connect((self.clock_sync, 0), (self.costas_loop, 0))
        self.connect((self.costas_loop, 0), (self.constellation_decoder, 0))
        self.connect((self.constellation_decoder, 0), (self.parser, 0))
        self.msg_connect(self.parser, "info", self, "feedback_out")

        # # self.connect((self.sync_word_correlator, 0), blocks.tag_debug(gr.sizeof_gr_complex, "corr_est", "corr_est"))
        # self.connect((self.sync_word_correlator, 0), blocks.file_sink(
        #     gr.sizeof_gr_complex, "debug/feedback-rx-after-corr.dat"))

        # src = blocks.vector_source_c(self.modulated_sync_word)
        # self.connect(src, blocks.file_sink(
        #     gr.sizeof_gr_complex, "debug/feedback-rx-sync-word.dat"))
