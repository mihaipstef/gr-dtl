#!/usr/bin/env python3


import struct
import sys


def headers(fname, start_offset):

    def _header(f):
        len_buf = f.read(4)
        if not len_buf:
            return None, None, None
        l = struct.unpack("i", len_buf)[0]
        n = struct.unpack("Q", f.read(8))[0]
        payload_offset = f.tell()
        #f.seek(payload_offset+len)
        f.read(l)
        return n, l, payload_offset

    with open(fname, 'rb') as f:
        f.seek(start_offset)
        for n, l, payload_offset in iter(lambda: _header(f), (None, None, None)):
            if n is None:
                break
            yield n, l, payload_offset


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
header_len = 12

# Detect offset where bots TX and RX are working
start_rx_offset = 0
start_tx_offset = 0
found_tx = None
for rx_n, rx_l, rx_offset in headers(rx_fname, 0):
    for tx_n, tx_l, tx_offset in headers(tx_fname, 0):
        if tx_n == rx_n:
            found_tx = tx_n
            break
    if found_tx is not None:
        start_rx_offset = rx_offset - header_len
        start_tx_offset = tx_offset - header_len
        break

print(f"Found transmission: no={found_tx}, tx_offset={start_tx_offset}, rx_offset={start_rx_offset}")

# Find matching offsets
offsets = []
tx_headers = {}
rx_headers = {}

mismatch_lens = 0
missing_tx = 0
bits_sent = 0
frames_sent = 0
frames_received = 0
bits_received = 0
missing_frames = 0
missing_bits = 0
frame_errors_count = 0
errors_count = 0

# Fetch TX headers
for tx_n, tx_len, tx_offset in headers(tx_fname, start_tx_offset):
    tx_headers[tx_n] = (tx_offset, tx_len)
    bits_sent += tx_len * 8
    frames_sent += 1

# Fetch RX headers
failed = []
for rx_n, rx_len, rx_offset in headers(rx_fname, start_rx_offset):
    if rx_n not in tx_headers:
        missing_tx += 1
        continue
    tx_offset, tx_len = tx_headers[rx_n]
    if tx_len != rx_len:
        mismatch_lens += 1
        print(rx_n, tx_len, rx_len, tx_offset, rx_offset)
        #break
        continue
    offsets.append((rx_offset + header_len, tx_offset + header_len, rx_len, rx_n))
    rx_headers[rx_n] = (rx_offset, rx_len)

for tx_n, tx_len, tx_offset in headers(tx_fname, start_tx_offset):
    if tx_n not in rx_headers:
        missing_frames += 1
        missing_bits += tx_len * 8


print(f"Errors: mismatch length={mismatch_lens}, missing txs={missing_tx}")
print(f"Matched frames:  {len(offsets)}")

crc_ok = 0
with open(tx_fname,"rb") as tx_f, open(rx_fname, "rb") as rx_f:
    for rx_offset, tx_offset, l, _ in offsets:
        tx_f.seek(tx_offset)
        rx_f.seek(rx_offset)
        tx_data = list(tx_f.read(l))
        rx_data = list(rx_f.read(l))
        e = count_errors(tx_data, rx_data)
        bits_received += l * 8
        frames_received += 1
        errors_count += e
        if e > 0:
            frame_errors_count += 1
        else:
            crc_ok += 1


print(f"Sent: frames={frames_sent}, bits={bits_sent}")
print(f"Received: frames={frames_received}, bits={bits_received}")
print(f"Frames: missed={missing_frames}, crc_ok={crc_ok}, crc_fail={frame_errors_count}")
print(f"BER (overall): {(errors_count + missing_bits)/bits_sent}")
print(f"BER (detected frames): {(errors_count)/bits_received}")
print(f"FER: {(frame_errors_count + missing_frames)/frames_sent}")

