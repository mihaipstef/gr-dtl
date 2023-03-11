#!/usr/bin/env python

import sys

def frames(fname):

    def _frame(f):
        len = f.read(4)
        n = f.read(4)
        buf = f.read(len)
        return n, buf

    with open(fname, 'rb') as f:
        for n, data in iter(lambda: _frame(f), (0, 0, b'')):
            yield n, data

def bits_mismatch(a, b):
    xored = a ^ b
    n = 0
    while (xored):
        xored = xored & (xored - 1)
        n += 1
    return n

def count_errors(tx_data, rx_data):
    errors = 0
    for tx_byte, rx_byte in zip(tx_data, rx_data):
        errors += bits_mismatch(tx_byte, rx_byte)
    return errors


tx_fname = sys.argv[1]
rx_fname = sys.argv[2]
if len(sys.argv) > 3:
    rx_search_count_max = int(sys.argv[3])
else:
    rx_search_count_max = 50

rx_search_count = 0
missing_frames_count = 0
missing_frames_bits_count = 0
errors_count = 0

bits_count = 0
frames_count = 0
for tx_n, tx_data in frames(tx_fname):
    bits_count += len(tx_data) * 8
    frames_count += 1

for rx_n, rx_data in frames(rx_fname):
    found = False
    rx_search_count = 0

    for tx_n, tx_data in frames(tx_fname):
        rx_search_count += 1
        if tx_n == rx_n:
            errors_count += count_errors(tx_data, rx_data)
            break
        else:
            missing_frames_count += 1
            missing_frames_bits_count += len(tx_data)
        if rx_search_count > rx_search_count_max:
            break