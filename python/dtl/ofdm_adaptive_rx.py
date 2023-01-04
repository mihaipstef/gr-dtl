from gnuradio import (
    analog,
    blocks,
    digital,
    dtl,
    fft,
    gr,
)


class ofdm_adaptive_rx(gr.hier_block2):
    """Adaptive OFDM Rx.
    """

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_rx",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_char))
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

        if [self.fft_len, self.fft_len] != [len(config.sync_word1), len(config.sync_word2)]:
            raise ValueError(
                "Length of sync sequence(s) must be FFT length.")
        self.sync_words = [config.sync_word1, config.sync_word2]

        if self.scramble_bits:
            self.scramble_seed = 0x7f
        else:
            self.scramble_seed = 0x00  # We deactivate the scrambler by init'ing it with zeros
        ### Sync ############################################################
        sync_detect = digital.ofdm_sync_sc_cfb(
            self.fft_len, self.cp_len, threshold=0.95)
        delay = blocks.delay(gr.sizeof_gr_complex, self.fft_len + self.cp_len)
        oscillator = analog.frequency_modulator_fc(-2.0 / self.fft_len)
        mixer = blocks.multiply_cc()
        hpd = digital.header_payload_demux(
            len(self.sync_words) + 1,
            self.fft_len, self.cp_len,
            self.frame_length_tag_key,
            "",
            True,
            header_padding=self.fft_len * 0
        )
        self.connect(self, sync_detect)
        self.connect(self, delay, (mixer, 0), (hpd, 0))
        self.connect((sync_detect, 0), oscillator, (mixer, 1))
        self.connect((sync_detect, 1), (hpd, 1))

        ### Header demodulation ##############################################
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

        ### Payload demod ####################################################
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
            symbols_skipped=1,  # (that was already in the header)
            alpha=0.1,
        )
        payload_eq = dtl.ofdm_adaptive_frame_equalizer_vcvc(
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
            payload_eq,
            payload_serializer,
            payload_demod,
            payload_pack,
            self.payload_descrambler,
            self.crc,
            self
        )

        if self.debug_log:
            self.connect((sync_detect, 0), blocks.file_sink(
                gr.sizeof_float, f"{config.debug_folder}/freq-offset.dat"))
            self.connect((sync_detect, 1), blocks.file_sink(
                gr.sizeof_char, f"{config.debug_folder}/sync-detect.dat"))
            self.connect((hpd, 0), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/rx-header.dat"))
            self.connect((hpd, 1), blocks.file_sinself.k(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/rx-payload.dat"))
            self.connect((chanest, 1), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/channel-estimate.dat"))
            self.connect((chanest, 0), blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-hdr-chanest.dat"))
            self.connect((chanest, 0), blocks.tag_debug(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-hdr-chanest.dat"))
            self.connect(header_eq, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-hdr-eq.dat"))
            self.connect(header_serializer, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/post-hdr-serializer.dat"))
            self.connect(self, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/pre-hpd-mixer.dat"))
            self.connect((hpd, 1), blocks.tag_debug(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-hpd.dat"))
            self.connect(payload_fft, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-payload-fft.dat"))
            self.connect(payload_eq, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/post-payload-eq.dat"))
            self.connect(payload_serializer, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/post-payload-serializer.dat"))
            self.connect(payload_demod, blocks.file_sink(
                1, f"{config.debug_folder}/post-payload-demod.dat"))
            self.connect(payload_pack, blocks.file_sink(
                1, f"{config.debug_folder}/post-payload-pack.dat"))
            self.connect(self.crc, blocks.file_sink(
                1, f"{config.debug_folder}/post-payload-crc.dat"))
