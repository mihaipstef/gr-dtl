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
    def from_parameters(cls, **kwargs):
        return cls(dtl.ofdm_adaptive_config.ofdm_adaptive_rx_config(**kwargs))

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_rx",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature.makev(3, 3, [gr.sizeof_char, gr.sizeof_gr_complex, gr.sizeof_char]))
        self.message_port_register_hier_out("feedback")

        self.fft_len = config.fft_len
        self.cp_len = config.cp_len
        self.frame_length_tag_key = config.frame_length_tag_key
        self.packet_length_tag_key = config.packet_length_tag_key
        self.packet_num_tag_key = config.packet_num_tag_key
        self.occupied_carriers = config.occupied_carriers
        self.pilot_carriers = config.pilot_carriers
        self.pilot_symbols = config.pilot_symbols
        self.scramble_bits = config.scramble_bits
        self.rolloff = config.rolloff
        self.debug_log = config.debug
        self.debug_folder = config.debug_folder

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

        # Synchronization
        self.sync_detect = digital.ofdm_sync_sc_cfb(
            self.fft_len, self.cp_len, threshold=0.99)
        self.delay = blocks.delay(gr.sizeof_gr_complex, self.fft_len + self.cp_len)
        self.oscillator = analog.frequency_modulator_fc(-2.0 / self.fft_len)
        self.mixer = blocks.multiply_cc()
        hpd = digital.header_payload_demux(
            len(self.sync_words) + 1,
            self.fft_len, self.cp_len,
            self.frame_length_tag_key,
            "",
            True,
            header_padding=self.fft_len * 0
        )
        self.connect((self, 0), self.sync_detect)
        self.connect((self, 0), self.delay, (self.mixer, 0), (hpd, 0))
        self.connect((self.sync_detect, 0), self.oscillator, (self.mixer, 1))
        self.connect((self.sync_detect, 1), (hpd, 1))

        # Header path
        header_fft = fft.fft_vcc(self.fft_len, True, (), True)
        chanest = digital.ofdm_chanest_vcvc(
            self.sync_words[0], self.sync_words[1], 1)
        header_constellation = digital.bpsk_constellation()
        header_equalizer = digital.ofdm_equalizer_simpledfe(
            self.fft_len,
            header_constellation.base(),
            self.occupied_carriers,
            self.pilot_carriers,
            self.pilot_symbols,
            symbols_skipped=0,
        )
        header_eq = digital.ofdm_frame_equalizer_vcvc(
            header_equalizer.base(),
            self.cp_len,
            self.frame_length_tag_key,
            True,
            1  # Header is 1 symbol long
        )
        header_serializer = digital.ofdm_serializer_vcc(
            self.fft_len, self.occupied_carriers,
            self.frame_length_tag_key
        )
        header_demod = digital.constellation_decoder_cb(
            header_constellation.base())
        header_formatter = dtl.ofdm_adaptive_packet_header(
            self.occupied_carriers, 1,
            self.packet_length_tag_key,
            self.frame_length_tag_key,
            self.packet_num_tag_key,
            1,  # BPSK
            scramble_header=self.scramble_bits
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
        payload_equalizer = dtl.ofdm_adaptive_equalizer(
            self.fft_len,
            [
                dtl.constellation_type_t.BPSK,
                dtl.constellation_type_t.QPSK,
                dtl.constellation_type_t.PSK8,
                dtl.constellation_type_t.QAM16,
            ],
            dtl.ofdm_adaptive_frame_snr_simple(0.1),
            self.occupied_carriers,
            self.pilot_carriers,
            self.pilot_symbols,
            symbols_skipped=1, # already in the header
            alpha=0.1,
        )
        self.payload_eq = dtl.ofdm_adaptive_frame_equalizer_vcvc(
            payload_equalizer.base(),
            dtl.ofdm_adaptive_feedback_decision(),
            self.cp_len,
            self.frame_length_tag_key,
            False,
            0,
        )
        payload_serializer = digital.ofdm_serializer_vcc(
            self.fft_len, self.occupied_carriers,
            self.frame_length_tag_key,
            self.packet_length_tag_key,
            1  # Skip 1 symbol (that was already in the header)
        )
        payload_demod = dtl.ofdm_adaptive_constellation_decoder_cb(
            [
                dtl.constellation_type_t.BPSK,
                dtl.constellation_type_t.QPSK,
                dtl.constellation_type_t.PSK8,
                dtl.constellation_type_t.QAM16,
            ],
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
        payload_pack = dtl.ofdm_adaptive_repack_bits_bb(
            self.packet_length_tag_key, False)
        self.crc = digital.crc32_bb(True, self.packet_length_tag_key)
        self.connect(
            (hpd, 1),
            payload_fft,
            self.payload_eq,
            payload_serializer,
            payload_demod,
            payload_pack,
            # self.payload_descrambler,
            # self.crc,
            (self, 0)
        )

        self.connect((self.sync_detect,1), (self, 2))

        if self.debug_log:
            self.connect((self.sync_detect, 1), blocks.file_sink(
                gr.sizeof_char, f"{self.debug_folder}/sync-detect.dat"))
            self.connect((hpd, 0), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/rx-header.dat"))
            self.connect((hpd, 1), blocks.file_sinself.k(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/rx-payload.dat"))
            self.connect((chanest, 1), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/channel-estimate.dat"))
            self.connect((chanest, 0), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-hdr-chanest.dat"))
            self.connect((chanest, 0), blocks.tag_debug(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-hdr-chanest.dat"))
            self.connect(header_eq, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-hdr-eq.dat"))
            self.connect(header_serializer, blocks.file_sink(
                gr.sizeof_gr_complex, f"{self.debug_folder}/post-hdr-serializer.dat"))
            self.connect(self, blocks.file_sink(
                gr.sizeof_gr_complex, f"{self.debug_folder}/pre-hpd-mixer.dat"))
            self.connect((hpd, 1), blocks.tag_debug(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-hpd.dat"))
            self.connect(payload_fft, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-payload-fft.dat"))
            self.connect(self.payload_eq, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{self.debug_folder}/post-payload-eq.dat"))
            self.connect(payload_serializer, blocks.file_sink(
                gr.sizeof_gr_complex, f"{self.debug_folder}/post-payload-serializer.dat"))
            self.connect(payload_demod, blocks.file_sink(
                1, f"{self.debug_folder}/post-payload-demod.dat"))
            self.connect(payload_pack, blocks.file_sink(
                1, f"{self.debug_folder}/post-payload-pack.dat"))
            self.connect(self.crc, blocks.file_sink(
                1, f"{self.debug_folder}/post-payload-crc.dat"))

    def _setup_feedback_tx(self):
        self.feedback_sps = 2
        self.feedback_nfilts = 32
        self.feedback_eb = 0.35
        self.feedback_psf_taps = filter.firdes.root_raised_cosine(self.feedback_nfilts, self.feedback_nfilts,
                                                        1.0, self.feedback_eb, 11*self.feedback_sps*self.feedback_nfilts)
        self.feedback_taps_per_filt = len(self.feedback_psf_taps) / self.feedback_nfilts
        self.feedback_filt_delay = int(1 + (self.feedback_taps_per_filt-1) // 2)
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
        self.feedback_mapper = digital.map_bb(self.feedback_constellation.pre_diff_code())
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

        self.msg_connect(self.payload_eq, "feedback_port", self.feedback_formatter, "in")
        self.msg_connect(self.payload_eq, "feedback_port", self, "feedback")

        self.msg_connect(self.feedback_formatter, "header",
                        self.feedback_to_tagged_stream, "pdus")
        self.connect(self.feedback_to_tagged_stream, self.feedback_repack_bits,
                    self.feedback_mapper, self.feedback_chunks_to_symbols, self.feedback_burst_shaper)
        self.connect(self.feedback_burst_shaper, self.feedback_resampler,
                    self.feedback_multiply_length_tag)
        self.connect((self.feedback_multiply_length_tag, 0), (self, 1))

        self.msg_connect(self.feedback_formatter, "header", blocks.message_debug(), "print")
