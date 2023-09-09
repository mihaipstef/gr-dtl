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

        # TX path
        self.tx = dtl.ofdm_transmitter(config)
        self.msg_connect(self.tx, "monitor", self, "monitor")
        self.connect(self, self.tx, self)

        # RX path

        self.rx = dtl.ofdm_receiver(config)
        self.connect(self, self.rx, (self, 0))
        self.connect((self.rx, 1), (self, 2))
        self.connect((self.rx, 2), (self, 3))
        self.connect((self.rx, 3), (self, 4))
        self.connect((self.rx, 4), (self, 5))
        self.connect((self.rx, 5), (self, 6))
        self.msg_connect(self.rx, "monitor", self, "monitor")
        self.msg_connect(self.rx, "feedback", self, "feedback")
        self.msg_connect(self.rx, "reverse_feedback", self, "feedback")

