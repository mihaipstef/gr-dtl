from gnuradio import (
    analog,
    blocks,
    digital,
    dtl,
    fft,
    filter,
    gr,
    pdu,
)
import pmt


class ofdm_adaptive_rx(gr.hier_block2):
    """Adaptive OFDM Rx.
    """

    @classmethod
    def from_parameters(cls, config_dict = None, **kwargs):
        cfg = dtl.ofdm_adaptive_config.make_rx_config(config_dict)
        cfg.__dict__.update(kwargs)
        return cls(cfg)

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_rx",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature.makev(7, 7, [gr.sizeof_char, gr.sizeof_gr_complex, gr.sizeof_char, gr.sizeof_char, gr.sizeof_gr_complex * config.fft_len, gr.sizeof_gr_complex, gr.sizeof_float]))
        self.message_port_register_hier_out("monitor")

        self.fft_len = config.fft_len
        self.cp_len = config.cp_len
        self.frame_length_tag_key = config.frame_length_tag_key
        self.packet_length_tag_key = config.packet_length_tag_key
        self.frame_no_tag_key = config.frame_no_tag_key
        self.occupied_carriers = config.occupied_carriers
        self.pilot_carriers = config.pilot_carriers
        self.pilot_symbols = config.pilot_symbols
        self.scramble_bits = config.scramble_bits
        self.rolloff = config.rolloff
        self.sync_threshold = config.sync_threshold
        self.frame_length = config.frame_length
        mc_schemes = list(zip(*config.mcs))[1]
        cnsts = list(zip(*mc_schemes))[0]
        self.constellations = list(set(cnsts))
        self.frame_store_fname = "/tmp/rx.dat" #f"{config.frame_store_folder}/rx.dat"
        self.use_sync_correct = config.use_sync_correct
        self.fec = len(config.fec_codes)
        self.codes_alist = []
        self.codes_id = {}
        if self.fec:
            self.codes_alist = list(zip(*config.fec_codes))[1]
            self.codes_id = { name: id+1 for (id, name) in enumerate(list(zip(*config.fec_codes))[0]) }
        self.mcs = [(snr_th, (cnst, self.codes_id.get(code_name, 0))) for (snr_th, (cnst, code_name)) in config.mcs]
        self.initial_mcs_id = config.initial_mcs_id

        if [self.fft_len, self.fft_len] != [len(config.sync_word1), len(config.sync_word2)]:
            raise ValueError(
                "Length of sync sequence(s) must be FFT length.")
        self.sync_words = [config.sync_word1, config.sync_word2]

        self._setup_ofdm_rx()
        self._setup_feedback_tx()

    def _setup_ofdm_rx(self):

        if self.scramble_bits:
            self.scramble_seed = 0x7f
        else:
            self.scramble_seed = 0x00  # We deactivate the scrambler by init'ing it with zeros

        header_len = 1
        if self.fec:
            header_len = 2

        # Synchronization
        self.sync_detect = digital.ofdm_sync_sc_cfb(
            self.fft_len, self.cp_len, threshold=self.sync_threshold)
        self.delay = blocks.delay(
            gr.sizeof_gr_complex, self.fft_len + self.cp_len)
        self.oscillator = analog.frequency_modulator_fc(-2.0 / self.fft_len)
        self.mixer = blocks.multiply_cc()
        sync_correct = dtl.ofdm_adaptive_frame_detect_bb(
            (self.frame_length +  len(self.sync_words) + header_len) * (self.fft_len + self.cp_len))
        hpd = digital.header_payload_demux(
            len(self.sync_words) + header_len,
            self.fft_len, self.cp_len,
            self.frame_length_tag_key,
            "",
            True,
            header_padding=self.fft_len * 0
        )
        freq_cmd_avg = blocks.moving_average_ff(1000, 1.0/1000, 4000, 1)
        self.connect((self, 0), self.sync_detect)
        self.connect((self, 0), self.delay, (self.mixer, 0), (hpd, 0))
        self.connect((self.sync_detect, 0), self.oscillator, (self.mixer, 1))
        if self.use_sync_correct:
            self.connect((self.sync_detect, 1), sync_correct, (hpd, 1))
            self.connect((self.sync_detect, 1), (self, 2))
            self.connect(sync_correct, (self, 3))
        else:
            self.connect((self.sync_detect, 1), (hpd, 1))
            self.connect((self.sync_detect, 1), (self, 2))
            self.connect((self.sync_detect, 1), (self, 3))


        # Header path
        header_fft = fft.fft_vcc(self.fft_len, True, (), True)
        chanest = digital.ofdm_chanest_vcvc(
            self.sync_words[0], self.sync_words[1], header_len)
        header_constellation = digital.bpsk_constellation()
        header_equalizer = dtl.ofdm_adaptive_header_equalizer(
            self.fft_len,
            header_constellation,
            dtl.ofdm_adaptive_frame_snr_simple(0.1),
            self.occupied_carriers,
            self.pilot_carriers,
            self.pilot_symbols,
            symbols_skipped=0,
            alpha=0.1,
        )
        header_eq = digital.ofdm_frame_equalizer_vcvc(
            header_equalizer.base(),
            self.cp_len,
            self.frame_length_tag_key,
            True,
            header_len
        )
        header_serializer = digital.ofdm_serializer_vcc(
            self.fft_len, self.occupied_carriers,
            self.frame_length_tag_key
        )
        header_demod = digital.constellation_decoder_cb(
            header_constellation.base())

        header_formatter = dtl.ofdm_adaptive_packet_header(
            [self.occupied_carriers[0] for _ in range(header_len)], header_len, self.frame_length,
            self.packet_length_tag_key,
            self.frame_length_tag_key,
            self.frame_no_tag_key,
            1,  # BPSK
            scramble_header=self.scramble_bits,
            has_fec=self.fec
        )

        header_parser = digital.packet_headerparser_b(
            header_formatter.formatter())
        self.connect(
            (hpd, 0),
            header_fft,
            chanest,
            header_eq,
            header_serializer,
            header_demod,
            header_parser
        )
        self.msg_connect(header_parser, "header_data", hpd, "header_data")

        # Payload path
        payload_fft = fft.fft_vcc(self.fft_len, True, (), True)

        payload_equalizer = dtl.ofdm_adaptive_payload_equalizer(
            self.fft_len,
            self.constellations,
            dtl.ofdm_adaptive_frame_snr_simple(0.1),
            self.occupied_carriers,
            self.pilot_carriers,
            self.pilot_symbols,
            symbols_skipped=header_len,  # already in the header
            alpha=0.1,
        )
        self.payload_eq = dtl.ofdm_adaptive_frame_equalizer_vcvc(
            payload_equalizer.base(),
            dtl.ofdm_adaptive_feedback_decision(2, 5, self.mcs, self.initial_mcs_id),
            self.cp_len,
            self.frame_length_tag_key,
            self.frame_no_tag_key,
            False,
            0,
        )

        payload_serializer = digital.ofdm_serializer_vcc(
            self.fft_len, self.occupied_carriers,
            self.frame_length_tag_key,
            self.packet_length_tag_key,
            1  # Skip 1 symbol (that was already in the header)
        )

        self.connect(
            (hpd, 1),
            payload_fft,
            (self.payload_eq, 0),
            payload_serializer,
        )

        if self.fec:
            ldpc_decs = dtl.make_ldpc_decoders(self.codes_alist)

            payload_demod = dtl.ofdm_adaptive_constellation_soft_cf(self.constellations, self.packet_length_tag_key)
            fec_dec = dtl.ofdm_adaptive_fec_decoder(
                ldpc_decs,
                dtl.ofdm_adaptive.frame_capacity(self.frame_length, self.occupied_carriers),
                dtl.ofdm_adaptive.max_bps(self.constellations),
                self.packet_length_tag_key
            )
            fec_pack =dtl.ofdm_adaptive_fec_pack_bb(self.packet_length_tag_key)
            self.connect(payload_demod, blocks.tag_debug(gr.sizeof_float, "payload_demod"))
            self.connect(
                payload_serializer,
                payload_demod,
                fec_dec,
                fec_pack,
                # self.payload_descrambler,
                (self, 0)
            )
            self.msg_connect(fec_dec, "monitor", self, "monitor")
        else:
            payload_demod = dtl.ofdm_adaptive_constellation_decoder_cb(
                self.constellations,
                self.packet_length_tag_key,
            )

            self.payload_descrambler = digital.additive_scrambler_bb(
                0x8a,
                self.scramble_seed,
                7,
                0,  # Don't reset after fixed length
                bits_per_byte=8,  # This is after packing
                reset_tag_key=self.packet_length_tag_key
            )
            payload_pack = dtl.ofdm_adaptive_frame_pack_bb(
                self.packet_length_tag_key, self.frame_no_tag_key, self.frame_store_fname)
            # self.connect(payload_demod, blocks.file_sink(gr.sizeof_char, "/tmp/rx_demod_frames.dat"))
            # self.connect(payload_demod, blocks.tag_debug(gr.sizeof_char, "demod"))
            #self.crc = digital.crc32_bb(True, self.packet_length_tag_key)
            self.connect(
                payload_serializer,
                payload_demod,
                payload_pack,
                # self.payload_descrambler,
                (self, 0)
            )
            self.msg_connect(payload_pack, "monitor", self, "monitor")

        self.connect((self.payload_eq, 0), (self, 4))
        self.connect((self.payload_eq, 1), (self, 5))
        self.connect((self.sync_detect, 0), (self, 6))
        self.msg_connect(self.payload_eq, "monitor", self, "monitor")


    def _setup_feedback_tx(self):
        self.feedback_sps = 2
        self.feedback_nfilts = 32
        self.feedback_eb = 0.35
        self.feedback_psf_taps = filter.firdes.root_raised_cosine(self.feedback_nfilts, self.feedback_nfilts,
                                                                  1.0, self.feedback_eb, 11*self.feedback_sps*self.feedback_nfilts)
        self.feedback_taps_per_filt = len(
            self.feedback_psf_taps) / self.feedback_nfilts
        self.feedback_filt_delay = int(
            1 + (self.feedback_taps_per_filt-1) // 2)
        self.feedback_format = dtl.ofdm_adaptive_feedback_format(
            digital.packet_utils.default_access_code, 0)
        self.feedback_constellation = digital.constellation_calcdist(digital.psk_2()[0], digital.psk_2()[1],
                                                                     2, 1, digital.constellation.AMPLITUDE_NORMALIZATION).base()
        self.feedback_constellation.gen_soft_dec_lut(8)
        self.feedback_bps = self.feedback_constellation.bits_per_symbol()

        self.feedback_to_tagged_stream = pdu.pdu_to_tagged_stream(
            gr.types.byte_t, "packet_len")
        self.feedback_formatter = digital.protocol_formatter_async(
            self.feedback_format.base())
        self.feedback_mapper = digital.map_bb(
            self.feedback_constellation.pre_diff_code())
        self.feedback_chunks_to_symbols = digital.chunks_to_symbols_bc(
            self.feedback_constellation.points(), 1)
        self.feedback_burst_shaper = digital.burst_shaper_cc(
            filter.firdes.window(fft.window.WIN_HANN, 50, 0), 0, self.feedback_filt_delay, True, "packet_len")
        self.feedback_repack_bits = blocks.repack_bits_bb(
            8, self.feedback_constellation.bits_per_symbol(), "packet_len", False, gr.GR_MSB_FIRST)
        self.feedback_resampler = filter.pfb.arb_resampler_ccf(
            self.feedback_sps,
            taps=self.feedback_psf_taps,
            flt_size=self.feedback_nfilts)
        self.feedback_resampler.declare_sample_delay(self.feedback_filt_delay)
        self.feedback_multiply_length_tag = blocks.tagged_stream_multiply_length(
            gr.sizeof_gr_complex*1, "packet_len", self.feedback_sps)

        self.msg_connect(self.payload_eq, "feedback_port",
                         self.feedback_formatter, "in")
        self.msg_connect(self.feedback_formatter, "header",
                         self.feedback_to_tagged_stream, "pdus")
        self.connect(self.feedback_to_tagged_stream, self.feedback_repack_bits,
                     self.feedback_mapper, self.feedback_chunks_to_symbols, self.feedback_burst_shaper)
        self.connect(self.feedback_burst_shaper, self.feedback_resampler,
                     self.feedback_multiply_length_tag)
        self.connect((self.feedback_multiply_length_tag, 0), (self, 1))

        self.msg_connect(self.feedback_formatter, "header",
                         blocks.message_debug(), "print")
