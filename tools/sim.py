#!/usr/bin/env python3

from gnuradio.filter import firdes
import sip
from gnuradio import analog
from gnuradio import blocks
from gnuradio import channels
from gnuradio import dtl
from gnuradio import gr
from gnuradio.fft import window
from gnuradio import zeromq
import json
import os
import signal


class ofdm_adaptive_sim(gr.top_block):

    def __init__(self, config_file, sent_frames = None, propagation_paths = [()], use_sync_correct = True, frame_length = 20, codes = []):
        gr.top_block.__init__(self, "OFDM Adaptive Simulator", catch_exceptions=True)
        self.samp_rate = samp_rate = 200000
        self.n_bytes = 100
        self.direct_channel_noise_level = 0.0001
        self.direct_channel_freq_offset = 0.5
        self.fft_len = 64
        self.cp_len = 16
        self.config_file = config_file
        self.use_sync_correct = use_sync_correct
        self.max_doppler = 0
        self.propagation_paths = propagation_paths
        self.frame_length = frame_length
        self.frame_samples = (self.frame_length + 4) * (self.fft_len + self.cp_len)
        self.codes = codes

        ##################################################
        # Blocks
        ##################################################

        #self.zeromq_pub = zeromq.pub_msg_sink('tcp://0.0.0.0:5552', 100, True)
        self.tx = dtl.ofdm_adaptive_tx.from_parameters(
            fft_len=self.fft_len,
            cp_len=self.cp_len,
            rolloff=0,
            scramble_bits=False,
            stop_no_input=False,
            frame_length=self.frame_length,
            codes_alist=self.codes,
             fec=len(self.codes),
        )
        self.rx = dtl.ofdm_adaptive_rx.from_parameters(
            fft_len=self.fft_len,
            cp_len=self.cp_len,
            rolloff=0,
            scramble_bits=False,
            use_sync_correct=self.use_sync_correct,
            frame_length=self.frame_length,
            codes_alist=self.codes,
            fec=len(self.codes),
        )
        delays, delays_std, delays_maxdev, mags = zip(*self.propagation_paths)
        self.fadding_channel = channels.selective_fading_model2(8, self.max_doppler, False, 4.0, 0, delays, delays_std, delays_maxdev, mags, 8)
        self.awgn_channel = channels.channel_model(
            noise_voltage=0.0,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0 + 1.0j],
            noise_seed=0,
            block_tags=True)
        self.throtle = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate, True)
        self.src = analog.sig_source_b(10000, analog.GR_SIN_WAVE, 100, 95, 0, 0)
        self.msg_debug = blocks.message_debug(True)

        ##################################################
        # Connections
        ##################################################

        if sent_frames is None:
            print(sent_frames)
            self.connect((self.src, 0), (self.tx, 0), (self.throtle, 0))
        else:
            self.connect((self.src, 0), (self.tx, 0), blocks.head(gr.sizeof_gr_complex, sent_frames * self.frame_samples), (self.throtle, 0))

        # Direct path
        self.connect(
            (self.throtle, 0),
            (self.fadding_channel, 0),
            (self.awgn_channel, 0),
            (self.rx, 0)
        )

        # Feedback path
        self.connect(
            (self.rx, 1),
            (self.tx, 1)
        )

        self.connect((self.rx, 0), blocks.null_sink(gr.sizeof_char))
        self.connect((self.rx, 2), blocks.null_sink(gr.sizeof_char))
        self.connect((self.rx, 5), blocks.null_sink(gr.sizeof_gr_complex))
        self.msg_connect((self.rx, "monitor"), (blocks.message_debug(True), "store"))
        self.msg_connect((self.tx, "monitor"), (self.msg_debug, "store"))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.throtle.set_sample_rate(self.samp_rate)
        self.fadding_channel.set_fDTs((0/self.samp_rate))

    def get_n_bytes(self):
        return self.n_bytes

    def set_n_bytes(self, n_bytes):
        self.n_bytes = n_bytes

    def set_direct_channel_noise_level(self, direct_channel_noise_level):
        self.direct_channel_noise_level = float(direct_channel_noise_level)
        self.awgn_channel.set_noise_voltage(self.direct_channel_noise_level)

    def set_direct_channel_freq_offset(self, direct_channel_freq_offset):
        self.direct_channel_freq_offset = direct_channel_freq_offset
        self.awgn_channel.set_frequency_offset(self.direct_channel_freq_offset)

    def set_max_doppler(self, val):
        self.max_doppler = val
        self.fadding_channel.set_fDTs(self.max_doppler)


def main(
    top_block_cls=ofdm_adaptive_sim,
    config_file="sim.run.json",
    sent_frames=None,
    propagation_paths = [()],
    use_sync_correct = True,
    frame_length = 20,
    codes = [],):

    tb = top_block_cls(
        config_file=config_file,
        sent_frames=sent_frames,
        propagation_paths=propagation_paths,
        use_sync_correct=use_sync_correct,
        frame_length=frame_length,
        codes=codes,)

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

    # To update parameters on the fly:
    # - define attribute to ofdm_adaptive_sim, eg new_attr
    # - implement setter, eg. set_new_attr
    def config_update(sig=None, frame=None):
        try:
            if "/" in tb.config_file:
                config_file = f"{tb.config_file}"
            else:
                config_file = f"{os.path.dirname(__file__)}/{tb.config_file}"
            print(f"Load: {config_file}")
            with open(config_file, "r") as f:
                content = f.read()
                config = json.loads(content)
                for k,v in config.items():
                    if (setter:=getattr(tb, f"set_{k}", None)) and getattr(tb, k):
                        print(f"update {k}={v}")
                        setter(v)
        except Exception as ex:
            print(f"Config file not found or broken ({tb.config_file})")
            print(str(ex))

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)
    signal.signal(signal.SIGHUP, config_update)

    config_update()

    tb.run()


if __name__ == '__main__':
    main()
