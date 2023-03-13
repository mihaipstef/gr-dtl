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
    debug: bool = False
    debug_folder: str = "/tmp/debug"
    scramble_bits: bool = False
    frame_length: int = 20
    constellations: t.Tuple[t.Tuple[float, dtl.constellation_type_t]] = ((sys.float_info.min, dtl.constellation_type_t.BPSK), (
        13, dtl.constellation_type_t.QPSK), (18, dtl.constellation_type_t.PSK8), (23, dtl.constellation_type_t.QAM16),)
    frame_store_fname: str = "/tmp"


@dc.dataclass
class ofdm_adaptive_tx_config(ofdm_adaptive_config):
    pass


@dc.dataclass
class ofdm_adaptive_rx_config(ofdm_adaptive_config):
    sync_threshold: float = 0.95
