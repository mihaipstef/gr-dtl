from gnuradio import (
    blocks,
    digital,
    dtl,
    fft,
    filter,
    gr,
    pdu,
)
import pmt


class feedback_adapter(gr.basic_block):

    def __init__(self):
        gr.basic_block.__init__(self,
                                name="feedback_adapter",
                                in_sig=[], out_sig=[])
        self.message_port_register_in(pmt.intern("dict"))
        self.message_port_register_out(pmt.intern("vec"))
        self.set_msg_handler(pmt.intern("dict"), self.handle_msg)

    def handle_msg(self, msg):

        vec = pmt.make_u8vector(2,0)
        pmt.u8vector_set(vec, 0, pmt.to_long(pmt.dict_ref(msg, dtl.feedback_constellation_key(), pmt.from_long(0))))
        pmt.u8vector_set(vec, 1, pmt.to_long(pmt.dict_ref(msg, dtl.fec_feedback_key(), pmt.from_long(0))))
        msg_vec = pmt.cons(pmt.PMT_NIL, vec)
        self.message_port_pub(pmt.intern("vec"), msg_vec)


class ofdm_adaptive_rx(gr.hier_block2):
    """Adaptive OFDM Simple modem (RX).
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

        # OFDM path
        self.direct_rx = dtl.ofdm_receiver(config)
        self.connect(self, self.direct_rx, (self, 0))
        self.connect((self.direct_rx, 1), (self, 2))
        self.connect((self.direct_rx, 2), (self, 3))
        self.connect((self.direct_rx, 3), (self, 4))
        self.connect((self.direct_rx, 4), (self, 5))
        self.connect((self.direct_rx, 5), (self, 6))
        self.msg_connect(self.direct_rx, "monitor", self, "monitor")

        # Feedback path
        self._setup_feedback_tx()


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
        self.adapter = feedback_adapter()

        self.msg_connect(self.direct_rx, "feedback",
                         self.adapter, "dict")
        self.msg_connect(self.adapter, "vec",
                         self.feedback_formatter, "in")
        self.msg_connect(self.feedback_formatter, "header",
                         self.feedback_to_tagged_stream, "pdus")
        self.connect(self.feedback_to_tagged_stream, self.feedback_repack_bits,
                     self.feedback_mapper, self.feedback_chunks_to_symbols, self.feedback_burst_shaper)
        self.connect(self.feedback_burst_shaper, self.feedback_resampler,
                     self.feedback_multiply_length_tag)
        self.connect((self.feedback_multiply_length_tag, 0), (self, 1))

