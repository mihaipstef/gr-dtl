import dataclasses as dc
from gnuradio import (
    digital,
    dtl,
)
import sys
import typing as t


@dc.dataclass
class ofdm_adaptive_config:
    fft_len: int = 64
    cp_len: int = 16
    frame_length_tag_key: str = "frame_length"
    packet_length_tag_key: str = "packet_length"
    packet_num_tag_key: str = "packet_num"
    occupied_carriers: t.Tuple[t.List[int]] = (list(range(-26, -21)) + list(range(-20, -7)) + list(
        range(-6, 0)) + list(range(1, 7)) + list(range(8, 21)) + list(range(22, 27)),)
    pilot_carriers: t.Tuple[t.Tuple[int]] = (
        (-21, -7, 7, 21,), (-21, -7, 7, 21,))
    pilot_sym_scramble_seq: t.Tuple[int] = (
        1, 1, 1, 1, -1, -1, -1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -
        1, -1, 1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1,
        1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, 1, -1, -1, -1, 1, -
        1, 1, -1, -1, 1, -1, -1, 1, 1, 1, 1, 1, -1, -1, 1, 1,
        -1, -1, 1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 1, -1, -1, -
        1, -1, 1, -1, -1, 1, -1, 1, 1, 1, 1, -1, 1, -1, 1, -1, 1,
        -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1, 1, 1, 1, -1, -
        1, 1, -1, -1, -1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1
    )
    pilot_symbols: t.Tuple[int] = tuple(
        [(x, x, x, -x) for x in pilot_sym_scramble_seq])
    sync_word1: list = digital.ofdm_txrx._make_sync_word1(
        fft_len, occupied_carriers, pilot_carriers)
    sync_word2: list = digital.ofdm_txrx._make_sync_word2(
        fft_len, occupied_carriers, pilot_carriers)
    rolloff: int = 0
    scramble_bits: bool = False
    frame_length: int = 20
    frame_store_folder: str = "/tmp"
    fec: bool = False
    fec_codes: t.Tuple[str] = () #(("fec_1", "n_0100_k_0027_gap_04.alist"))
    mcs: t.Tuple[t.Tuple[float, t.Tuple[dtl.constellation_type_t, str]]] = ((sys.float_info.min, (dtl.constellation_type_t.BPSK, "fec_1")), (
        11, (dtl.constellation_type_t.QPSK, "fec_1")), (12, (dtl.constellation_type_t.PSK8, "fec_1")), (23, (dtl.constellation_type_t.QAM16, "fec_1")),)


@dc.dataclass
class ofdm_adaptive_tx_config(ofdm_adaptive_config):
    stop_no_input: bool = False


@dc.dataclass
class ofdm_adaptive_rx_config(ofdm_adaptive_config):
    sync_threshold: float = 0.95
    use_sync_correct: bool = True


def _make_config(cfg, json_dict, parser):

    class _default_parser:
        def mcs(cls, v):
            cnsts = {
                "bpsk": dtl.constellation_type_t.BPSK,
                "qpsk": dtl.constellation_type_t.QPSK,
                "psk8": dtl.constellation_type_t.PSK8,
                "qam16": dtl.constellation_type_t.QAM16,
            }
            return [(snr, (cnsts[cnst], fec)) for (snr, (cnst, fec)) in v]

    if json_dict:
        for key, val in json_dict.items():
            if key in cfg.__dict__:
                if key in parser.__dict__:
                    cfg.__setattr__(key, parser.__dict__[key](parser, val))
                elif key in _default_parser.__dict__:
                    cfg.__setattr__(key, _default_parser.__dict__[key](_default_parser, val))
                else:
                    cfg.__setattr__(key, val)
    return cfg


def make_tx_config(json_dict):
    class _parser:
        pass
    return _make_config(ofdm_adaptive_tx_config(), json_dict, _parser)


def make_rx_config(json_dict):
    class _parser:
        pass
    return _make_config(ofdm_adaptive_rx_config(), json_dict, _parser)

