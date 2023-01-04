from gnuradio import (
    gr,
    blocks,
    fft,
    digital,
    dtl,
)


class ofdm_adaptive_tx(gr.hier_block2):
    """Adaptive OFDM Tx
    """

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_tx",
                                gr.io_signature(1, 1, gr.sizeof_char),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
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
        self.debug_log = config.debug

        if [self.fft_len, self.fft_len] != [len(config.sync_word1), len(config.sync_word2)]:
            raise ValueError("Length of sync sequence(s) must be FFT length.")
        self.sync_words = [config.sync_word1, config.sync_word2]

        if config.scramble_bits:
            self.scramble_seed = 0x7f
        else:
            self.scramble_seed = 0x00  # We deactivate the scrambler by init'ing it with zeros

        ### Header modulation ################################################
        crc = digital.crc32_bb(False, self.packet_length_tag_key)
        header_constellation = digital.constellation_bpsk()
        header_mod = digital.chunks_to_symbols_bc(
            header_constellation.points())
        formatter_object = dtl.ofdm_adaptive_packet_header(
            self.occupied_carriers, 1,
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
            self,
            crc,
            header_gen,
            header_mod,
            (header_payload_mux, 0)
        )
        ### Payload modulation ###############################################
        payload_mod = dtl.ofdm_adaptive_chunks_to_symbols_bc(
            [
                dtl.constellation_type_t.BPSK,
                dtl.constellation_type_t.QPSK,
                dtl.constellation_type_t.PSK8,
                dtl.constellation_type_t.QAM16,
            ],
            self.packet_length_tag_key
        )
        payload_scrambler = digital.additive_scrambler_bb(
            0x8a,
            self.scramble_seed,
            7,
            0,  # Don't reset after fixed length (let the reset tag do that)
            bits_per_byte=8,  # This is before unpacking
            reset_tag_key=self.packet_length_tag_key
        )
        payload_unpack = dtl.ofdm_adaptive_repack_bits_bb(
            self.packet_length_tag_key
        )
        self.connect(
            crc,
            payload_scrambler,
            payload_unpack,
            payload_mod,
            (header_payload_mux, 1)
        )
        ### Create OFDM frame ################################################
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

        if self.debug_log:
            self.connect(header_gen, blocks.file_sink(
                1, f"{config.debug_folder}/tx-hdr.dat"))
            self.connect(header_mod, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/tx-header-pre-mux.dat"))
            self.connect(payload_mod, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/tx-payload-pre-mux.dat"))
            self.connect(header_payload_mux, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/tx-header-payload-mux.dat"))
            self.connect(allocator, blocks.file_sink(
                gr.sizeof_gr_complex * self.fft_len, f"{config.debug_folder}/tx-post-allocator.dat"))
            self.connect(cyclic_prefixer, blocks.file_sink(
                gr.sizeof_gr_complex, f"{config.debug_folder}/tx-signal.dat"))
