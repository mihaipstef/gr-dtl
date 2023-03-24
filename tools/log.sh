#!/bin/bash

script_dir="$(dirname $0)"

tx_log=$1
rx_log=$2

received=$(cat $rx_log | grep "header_parser" | wc -l)
sent=$(cat $tx_log | grep header_formatter | wc -l)
crc_fail=$(cat $rx_log | grep "header_parser" | grep "crc=failed" | wc -l)
missed_frames=$((sent - received))
payload_crc_fail=$(cat $rx_log | grep "ofdm_adaptive_frame_pack" | grep "crc_ok=false" | wc -l)
payload_crc_success=$(cat $rx_log | grep "ofdm_adaptive_frame_pack" | grep "crc_ok=true" | wc -l)
snr_stats=$(cat $rx_log | grep SNRest | awk -F ', ' '{print $2}' | awk -F ':' '{print $2}' | $script_dir/stats.r)


printf "Missed frames rate: %.2f (%d/%d)\n" $((10**2*missed_frames/sent))e-2 $missed_frames $sent
printf "Header CRC failed: %d\n" $crc_fail
printf "Payload CRC success: %d\n" $payload_crc_success
printf "Payload CRC fail: %d\n" $payload_crc_fail
printf "Frame success rate: %.2f (%d/%d)\n" $((10**2*payload_crc_success/sent))e-2 $payload_crc_success $sent
printf "SNR (min, max, median, mean): %s\n" $snr_stats

