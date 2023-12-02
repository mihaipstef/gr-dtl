from gnuradio import (
    gr,
    blocks,
    digital,
    dtl,
    filter,
)


class ofdm_adaptive_tx(gr.hier_block2):
    """Adaptive OFDM Simplex Tx
    """

    @classmethod
    def from_parameters(cls, config_dict = None, **kwargs):
        cfg = dtl.ofdm_adaptive_config.make_tx_config(config_dict)
        cfg.__dict__.update(kwargs)
        return cls(cfg)


    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_tx",
                                gr.io_signature.makev(
                                    2, 2, [gr.sizeof_char, gr.sizeof_gr_complex]),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))

        self.message_port_register_hier_out("monitor")

        # OFDM path
        self.direct_tx = dtl.ofdm_transmitter(config)
        self.msg_connect(self.direct_tx, "monitor", self, "monitor")
        self.connect(self, self.direct_tx, self)

        # Feedback path
        self._setup_feedback_rx()


    def _setup_feedback_rx(self):
        self.feedback_sps = 2  # samples per symbol
        self.feedback_eb = 0.35  # excess bw
        self.feedback_nfilts = 32
        self.feedback_threshold = 0
        self.feedback_taps = filter.firdes.root_raised_cosine(self.feedback_nfilts, self.feedback_nfilts,
                                                              1.0, self.feedback_eb, 11*self.feedback_sps*self.feedback_nfilts)
        self.feedback_format = dtl.ofdm_adaptive_feedback_format(
            digital.packet_utils.default_access_code, self.feedback_threshold)
        self.feedback_constellation = digital.constellation_calcdist(digital.psk_2()[0], digital.psk_2()[1],
                                                                     2, 1, digital.constellation.AMPLITUDE_NORMALIZATION).base()
        self.feedback_rxmod = digital.generic_mod(
            self.feedback_constellation, False, self.feedback_sps, True, self.feedback_eb, False, False)
        # digital.packet_utils.default_access_code bytes
        self.feedback_preamble = [0xac, 0xdd,
                                  0xa4, 0xe2, 0xf2, 0x8c, 0x20, 0xfc]
        self.feedback_mark_delays = [0, 0, 34, 56, 87, 119]
        self.feedback_modulated_sync_word = digital.modulate_vector_bc(
            self.feedback_rxmod.to_basic_block(), self.feedback_preamble, [1])
        self.feedback_mark_delay = self.feedback_mark_delays[self.feedback_sps]

        self.feedback_parser = digital.protocol_parser_b(self.feedback_format)

        self.feedback_clock_sync = digital.pfb_clock_sync_ccf(
            self.feedback_sps, 6.28/400.0, self.feedback_taps, self.feedback_nfilts, float(self.feedback_nfilts/2), 1.5, 1)

        self.feedback_costas_loop = digital.costas_loop_cc(
            (6.28/200.0), self.feedback_constellation.arity(), False)
        self.feedback_sync_word_correlator = digital.corr_est_cc(
            self.feedback_modulated_sync_word, self.feedback_sps, self.feedback_mark_delay, 0.9, digital.THRESHOLD_ABSOLUTE)
        self.feedback_constellation_decoder = digital.constellation_decoder_cb(
            self.feedback_constellation)
        self.feedback_amp_est_multiplier = blocks.multiply_by_tag_value_cc(
            "amp_est", 1)

        self.connect((self, 1), (self.feedback_sync_word_correlator, 0))
        # self.msg_connect(self.feedback_sync_word_correlator, "header", blocks.message_debug(), "print")
        self.connect((self.feedback_sync_word_correlator, 0),
                     (self.feedback_amp_est_multiplier, 0))
        self.connect((self.feedback_amp_est_multiplier, 0),
                     (self.feedback_clock_sync, 0))
        self.connect((self.feedback_clock_sync, 0),
                     (self.feedback_costas_loop, 0))
        self.connect((self.feedback_costas_loop, 0),
                     (self.feedback_constellation_decoder, 0))
        self.connect((self.feedback_constellation_decoder, 0),
                     (self.feedback_parser, 0))
        self.msg_connect(self.feedback_parser, "info",
                         self, "monitor")
        self.msg_connect(self.feedback_parser, "info",
                             self.direct_tx, "feedback")


    def set_feedback(self, constellation, fec_scheme=None):
        self.direct_tx.set_feedback(constellation, fec_scheme)