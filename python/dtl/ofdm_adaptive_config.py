from gnuradio import (
    digital,
)

class ofdm_adaptive_default_config:
    fft_len = 64
    cp_len = 16
    frame_length_tag_key = "frame_length"
    packet_length_tag_key = "packet_length"
    packet_num_tag_key = "packet_num"
    occupied_carriers = (list(range(-26, -21)) + list(range(-20, -7)) + list(
        range(-6, 0)) + list(range(1, 7)) + list(range(8, 21)) + list(range(22, 27)),)
    pilot_carriers = ((-21, -7, 7, 21,), (-21, -7, 7, 21,))
    pilot_sym_scramble_seq = (
        1, 1, 1, 1, -1, -1, -1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -
        1, -1, 1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1,
        1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, 1, -1, -1, -1, 1, -
        1, 1, -1, -1, 1, -1, -1, 1, 1, 1, 1, 1, -1, -1, 1, 1,
        -1, -1, 1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 1, -1, -1, -
        1, -1, 1, -1, -1, 1, -1, 1, 1, 1, 1, -1, 1, -1, 1, -1, 1,
        -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1, 1, 1, 1, -1, -
        1, 1, -1, -1, -1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1
    )
    pilot_symbols = tuple([(x, x, x, -x) for x in pilot_sym_scramble_seq])
    sync_word1 = digital.ofdm_txrx._make_sync_word1(
        fft_len, occupied_carriers, pilot_carriers)
    sync_word2 = digital.ofdm_txrx._make_sync_word2(
        fft_len, occupied_carriers, pilot_carriers)
    rolloff = 0
    debug = False
    debug_folder = "debug"
    scramble_bits = False
