from gnuradio import (
    gr,
    blocks,
    digital,
    dtl,
    filter,
)


class ofdm_adaptive_full_duplex(gr.hier_block2):
    """Adaptive OFDM Full Duplex
    """

    @classmethod
    def from_parameters(cls, config_dict = None, **kwargs):
        cfg = dtl.ofdm_adaptive_config.make_full_duplex_config(config_dict)
        cfg.__dict__.update(kwargs)
        return cls(cfg)


    def __init__(self, config):
        gr.hier_block2.__init__(self, "ofdm_adaptive_full_duplex",
                                gr.io_signature.makev(
                                    2, 2, [gr.sizeof_char, gr.sizeof_gr_complex]),
                                gr.io_signature.makev(2, 2, [gr.sizeof_char,gr.sizeof_gr_complex]))

        self.message_port_register_hier_out("monitor")

        # TX path
        self.tx = dtl.ofdm_transmitter(config)
        self.msg_connect(self.tx, "monitor", self, "monitor")
        self.connect((self, 0), (self.tx, 0), (self, 1))

        # RX path
        self.rx = dtl.ofdm_receiver(config)
        self.connect((self, 1), (self.rx, 0), (self, 0))
        self.connect((self.rx, 1), blocks.null_sink(gr.sizeof_char))
        self.connect((self.rx, 3), blocks.null_sink(gr.sizeof_gr_complex * config.fft_len))

        self.msg_connect(self.rx, "header", self.tx, "header")
        self.msg_connect(self.rx, "feedback", self.tx, "feedback")

        self.msg_connect(self.rx, "monitor", self, "monitor")


