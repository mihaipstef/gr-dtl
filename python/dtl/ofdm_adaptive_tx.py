from gnuradio import (
    gr,
    blocks,
    fft,
    digital,
    dtl,
    filter,
)
import pmt


class ofdm_adaptive_tx(gr.hier_block2):
    """Adaptive OFDM Tx
    """

    @classmethod
    def from_parameters(cls, **kwargs):
        return cls(dtl.ofdm_adaptive_config.ofdm_adaptive_tx_config(**kwargs))

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_tx",
                                gr.io_signature.makev(
                                    2, 2, [gr.sizeof_char, gr.sizeof_gr_complex]),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))

        self.message_port_register_hier_out("monitor")

        self.fft_len = config.fft_len
        self.cp_len = config.cp_len
        self.packet_length_tag_key = config.packet_length_tag_key
        self.frame_length_tag_key = config.frame_length_tag_key
        self.packet_num_tag_key = config.packet_num_tag_key
        self.occupied_carriers = config.occupied_carriers
        self.pilot_carriers = config.pilot_carriers
        self.pilot_symbols = config.pilot_symbols
        self.scramble_bits = config.scramble_bits
        self.rolloff = config.rolloff
        self.frame_length = config.frame_length
        self.payload_length_tag_key = "payload_length"
        self.constellations = config.constellations
        self.frame_store_fname = f"{config.frame_store_folder}/tx.dat"
        self.stop_no_input = config.stop_no_input

        if [self.fft_len, self.fft_len] != [len(config.sync_word1), len(config.sync_word2)]:
            raise ValueError("Length of sync sequence(s) must be FFT length.")
        self.sync_words = [config.sync_word1, config.sync_word2]

        if config.scramble_bits:
            self.scramble_seed = 0x7f
        else:
            self.scramble_seed = 0x00  # We deactivate the scrambler by init'ing it with zeros

        self._setup_direct_tx()
        self._setup_feedback_rx()

    def _setup_direct_tx(self):

        self.frame_unpack = dtl.ofdm_adaptive_frame_bb(
            self.packet_length_tag_key, list(zip(*self.constellations))[1], self.frame_length,
            len(self.occupied_carriers[0]), self.frame_store_fname, self.stop_no_input)

        # Header path blocks
        header_constellation = digital.constellation_bpsk()
        header_mod = digital.chunks_to_symbols_bc(
            header_constellation.points())
        formatter_object = dtl.ofdm_adaptive_packet_header(
            self.occupied_carriers, 1, self.frame_length,
            self.packet_length_tag_key,
            self.frame_length_tag_key,
            self.packet_num_tag_key,
            bits_per_header_sym=1,  # BPSK
            scramble_header=self.scramble_bits
        )
        header_gen = digital.packet_headergenerator_bb(
            formatter_object.base(), self.packet_length_tag_key)
        header_payload_mux = blocks.tagged_stream_mux(
            itemsize=gr.sizeof_gr_complex * 1,
            lengthtagname=self.packet_length_tag_key,
            tag_preserve_head_pos=1  # Head tags on the payload stream stay on the head
        )
        self.connect(
            self.frame_unpack,
            header_gen,
            header_mod,
            (header_payload_mux, 0)
        )
        # Payload path blocks
        payload_mod = dtl.ofdm_adaptive_chunks_to_symbols_bc(
            list(zip(*self.constellations))[1],
            self.packet_length_tag_key
        )
        # payload_scrambler = digital.additive_scrambler_bb(
        #     0x8a,
        #     self.scramble_seed,
        #     7,
        #     0,  # Don't reset after fixed length (let the reset tag do that)
        #     bits_per_byte=8,  # This is before unpacking
        #     reset_tag_key=self.packet_length_tag_key
        # )

        self.connect(
            (self, 0),
            # payload_scrambler,
            self.frame_unpack,
            payload_mod,
            (header_payload_mux, 1)
        )

        # OFDM blocks
        allocator = digital.ofdm_carrier_allocator_cvc(
            self.fft_len,
            occupied_carriers=self.occupied_carriers,
            pilot_carriers=self.pilot_carriers,
            pilot_symbols=self.pilot_symbols,
            sync_words=self.sync_words,
            len_tag_key=self.packet_length_tag_key
        )
        ffter = fft.fft_vcc(
            self.fft_len,
            False,  # Inverse FFT
            (),  # No window
            True  # Shift
        )
        cyclic_prefixer = digital.ofdm_cyclic_prefixer(
            self.fft_len,
            self.fft_len + self.cp_len,
            self.rolloff,
            self.packet_length_tag_key
        )

        self.connect(header_payload_mux, allocator,
                     ffter, cyclic_prefixer, self)

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
                         self.frame_unpack, "feedback")
        self.msg_connect(self.feedback_parser, "info",
                         self, "monitor")
        self.msg_connect(self.frame_unpack, "monitor", self, "monitor")

    def set_constellation(self, constellation):
        self.frame_unpack.set_constellation(constellation)
