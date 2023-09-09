from gnuradio import (
    gr,
    blocks,
    fft,
    digital,
    dtl,
)
import pmt


class ofdm_transmitter(gr.hier_block2):
    """Adaptive OFDM Transmitter.
    """

    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_tx",
                                gr.io_signature.makev(
                                    1, 1, [gr.sizeof_char]),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
        # Config
        self.sample_rate = config.sample_rate
        self.fft_len = config.fft_len
        self.cp_len = config.cp_len
        self.packet_length_tag_key = config.packet_length_tag_key
        self.frame_length_tag_key = config.frame_length_tag_key
        self.frame_no_tag_key = config.frame_no_tag_key
        self.occupied_carriers = config.occupied_carriers
        self.pilot_carriers = config.pilot_carriers
        self.pilot_symbols = config.pilot_symbols
        self.scramble_bits = config.scramble_bits
        self.rolloff = config.rolloff
        self.frame_length = config.frame_length
        self.payload_length_tag_key = "payload_length"
        self.mc_schemes = list(zip(*config.mcs))[1]
        cnsts = list(zip(*self.mc_schemes))[0]
        self.constellations = list(set(cnsts))
        self.frame_store_fname = f"{config.frame_store_folder}/tx.dat"
        self.fec = len(config.fec_codes)
        self.codes_alist = []
        if self.fec:
            self.codes_alist = list(zip(*config.fec_codes))[1]
            self.codes_id = { name: id+1 for (id, name) in enumerate(list(zip(*config.fec_codes))[0]) }
        self.initial_mcs = self.mc_schemes[config.initial_mcs_id]

        if [self.fft_len, self.fft_len] != [len(config.sync_word1), len(config.sync_word2)]:
            raise ValueError("Length of sync sequence(s) must be FFT length.")
        self.sync_words = [config.sync_word1, config.sync_word2]

        if config.scramble_bits:
            self.scramble_seed = 0x7f
        else:
            self.scramble_seed = 0x00  # We deactivate the scrambler by init'ing it with zeros
        self.max_empty_frames = config.max_empty_frames

        self.message_port_register_hier_out("monitor")
        self.message_port_register_hier_in("feedback")
        self.message_port_register_hier_in("reverse_feedback")

        self._setup()


    def _setup(self):
        # Header path blocks
        header_constellation = digital.constellation_bpsk()
        header_mod = digital.chunks_to_symbols_bc(
            header_constellation.points())

        header_len = 1
        if self.fec:
            header_len = 2

        frame_capacity = dtl.ofdm_adaptive.frame_capacity(
                self.frame_length, self.occupied_carriers)
        frame_rate = self.sample_rate / ((self.frame_length + header_len + len(self.sync_words)) * (self.fft_len + self.cp_len))

        header = dtl.ofdm_adaptive_packet_header(
            [self.occupied_carriers[0]
                for _ in range(header_len)], header_len, self.frame_length,
            self.packet_length_tag_key,
            self.frame_length_tag_key,
            self.frame_no_tag_key,
            bits_per_header_sym=1,  # BPSK
            scramble_header=self.scramble_bits,
            has_fec=self.fec
        )
        header_gen = digital.packet_headergenerator_bb(
            header.base(), self.packet_length_tag_key)
        header_payload_mux = blocks.tagged_stream_mux(
            itemsize=gr.sizeof_gr_complex * 1,
            lengthtagname=self.packet_length_tag_key,
            tag_preserve_head_pos=1  # Head tags on the payload stream stay on the head
        )

        # Payload path blocks
        payload_mod = dtl.ofdm_adaptive_chunks_to_symbols_bc(
            self.constellations,
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
        self.to_stream = dtl.ofdm_adaptive_frame_to_stream_vbb(frame_capacity, self.packet_length_tag_key)
        if self.fec:
            self.ldpc_encs = dtl.make_ldpc_encoders(self.codes_alist)
            repack = blocks.repack_bits_bb(8, 1)
            self.fec_frame = dtl.ofdm_adaptive_fec_frame_bvb(self.ldpc_encs,
                                                             frame_capacity,
                                                             frame_rate,
                                                             dtl.ofdm_adaptive.max_bps(
                                                                 self.constellations),
                                                             self.max_empty_frames,
                                                             self.packet_length_tag_key)

            # set initial MCS scheme
            self.set_feedback(self.initial_mcs[0], self.initial_mcs[1])

            self.connect(
                (self, 0),
                # payload_scrambler,
                repack,
                self.fec_frame,
                self.to_stream,
                payload_mod,
                (header_payload_mux, 1)
            )

            self.connect(
                self.to_stream,
                header_gen,
                header_mod,
                (header_payload_mux, 0)
            )
        else:
            # HACK: Adding a repack just before frame builder improves scheduler performance (not sure why)
            repack = blocks.repack_bits_bb(8, 8)

            self.frame_unpack = dtl.ofdm_adaptive_frame_bb(
                self.packet_length_tag_key,
                self.constellations, self.frame_length, frame_rate,
                len(self.occupied_carriers[0]), self.frame_store_fname, self.max_empty_frames)
            self.set_feedback(self.initial_mcs[0])
            self.connect(
                self.to_stream,
                header_gen,
                header_mod,
                (header_payload_mux, 0)
            )
            self.connect(
                (self, 0),
                repack,
                self.frame_unpack,
                self.to_stream,
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

        if self.fec:
            self.msg_connect(self, "feedback",
                             self.fec_frame, "feedback")
            self.msg_connect(self, "reverse_feedback",
                             self.fec_frame, "reverse_feedback")
            self.msg_connect(self.fec_frame, "monitor", self, "monitor")
        else:
            self.msg_connect(self, "feedback",
                             self.frame_unpack, "feedback")
            self.msg_connect(self, "reverse_feedback",
                             self.frame_unpack, "reverse_feedback")
            self.msg_connect(self.frame_unpack, "monitor", self, "monitor")


    def set_feedback(self, constellation, fec_scheme=None):
        if not self.fec:
            self.frame_unpack.set_constellation(constellation)
        else:
            assert(fec_scheme is not None and fec_scheme in self.codes_id)
            feedback = pmt.make_dict()
            feedback = pmt.dict_add(feedback, dtl.fec_feedback_key(), pmt.from_long(self.codes_id[fec_scheme]))
            feedback = pmt.dict_add(feedback, dtl.feedback_constellation_key(), pmt.from_long(int(constellation)))
            self.fec_frame.process_feedback(feedback)

