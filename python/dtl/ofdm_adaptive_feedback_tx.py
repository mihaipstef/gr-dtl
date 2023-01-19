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


class ofdm_adaptive_feedback_tx(gr.hier_block2):
    """Adaptive OFDM feedback TX
    """

    def __init__(self):
        gr.hier_block2.__init__(self, "ofdm_adaptive_feedback_tx",
                                gr.io_signature(1, 1, gr.sizeof_char),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))

        self.message_port_register_hier_in("feedback_in")

        self.sps = 2
        self.nfilts = 32
        self.eb = 0.35
        self.psf_taps = filter.firdes.root_raised_cosine(self.nfilts, self.nfilts,
                                                         1.0, self.eb, 11*self.sps*self.nfilts)
        self.taps_per_filt = len(self.psf_taps) / self.nfilts
        self.filt_delay = int(1 + (self.taps_per_filt-1) // 2)
        self.format = dtl.ofdm_adaptive_feedback_format(
            digital.packet_utils.default_access_code, 0)
        self.constellation = digital.constellation_calcdist(digital.psk_2()[0], digital.psk_2()[1],
                                                            2, 1, digital.constellation.AMPLITUDE_NORMALIZATION).base()
        self.constellation.gen_soft_dec_lut(8)
        self.bps = self.constellation.bits_per_symbol()

        self.to_tagged_stream = pdu.pdu_to_tagged_stream(
            gr.types.byte_t, "packet_len")
        self.formatter = digital.protocol_formatter_async(
            self.format.base())
        self.mapper = digital.map_bb(self.constellation.pre_diff_code())
        self.chunks_to_symbols = digital.chunks_to_symbols_bc(
            self.constellation.points(), 1)
        self.burst_shaper = digital.burst_shaper_cc(
            filter.firdes.window(fft.window.WIN_HANN, 20, 0), 0, self.filt_delay, True, "packet_len")
        self.repack_bits = blocks.repack_bits_bb(
            8, self.constellation.bits_per_symbol(), "packet_len", False, gr.GR_MSB_FIRST)
        self.resampler = filter.pfb.arb_resampler_ccf(
            self.sps,
            taps=self.psf_taps,
            flt_size=self.nfilts)
        self.resampler.declare_sample_delay(self.filt_delay)
        self.multiply_length_tag = blocks.tagged_stream_multiply_length(
            gr.sizeof_gr_complex*1, "packet_len", self.sps)

        self.connect(self, self.formatter)
        self.msg_connect(self.formatter, "header",
                         self.to_tagged_stream, "pdus")
        self.connect(self.to_tagged_stream, self.repack_bits,
                     self.mapper, self.chunks_to_symbols, self.burst_shaper)
        self.connect(self.burst_shaper, self.resampler,
                     self.multiply_length_tag, self)

        # self.connect(self.to_tagged_stream, blocks.file_sink(
        #     gr.sizeof_char, "debug/feedback-tx-after-format.dat"))
        # self.connect(self.chunks_to_symbols, blocks.file_sink(
        #     gr.sizeof_gr_complex, "debug/feedback-tx-symbols.dat"))
        # self.connect(self.burst_shaper, blocks.file_sink(
        #     gr.sizeof_gr_complex, "debug/feedback-tx-symbols-shapped.dat"))
        # self.connect(self.resampler, blocks.file_sink(
        #     gr.sizeof_gr_complex, "debug/feedback-tx-symbols-resampled.dat"))

    def _post(self, port, msg):
        """Pass single message to the first block in the chain (formatter).
        """
        if self.message_port_is_hier_in(port):
            self.formatter._post(pmt.intern("in"), msg)
