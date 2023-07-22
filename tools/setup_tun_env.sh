#!/bin/bash

tun0_ip="2.2.2.2"
tun1_ip="3.3.3.3"

# setup 2 tun interfaces
ip tuntap add mode tun tun0
ip addr add $tun0_ip/32 dev tun0
ip link set dev tun0 up
ip tuntap add mode tun tun1
ip addr add $tun1_ip/32 dev tun1
ip link set dev tun1 up

# remove default local routes
ip ro del $tun0_ip ta local
ip ro del $tun1_ip ta local

# add local routes to accept incoming traffic
# (make the packets be treated as addressed to the local host and processed as such)
ip ro add local $tun0_ip dev tun0 table 19
ip ru add iif tun0 lookup 19
ip ro add local $tun1_ip dev tun1 table 19
ip ru add iif tun1 lookup 19

# P2P routes
ip ro add $tun1_ip/32 dev tun0
ip ro add $tun0_ip/32 dev tun1
