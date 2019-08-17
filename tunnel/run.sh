#!/bin/bash
export PATH=$PATH:/sd/bridge-utils-1.6/build/sbin:/sd/ipip/tunctl-1.5
./test_tunnel 192.168.10.10 192.168.10.11 &
brctl addbr br0
brctl addif br0 eth0
brctl addif br0 tap0
ip link set br0 up

brctl addbr br1
brctl addif br1 eth1
brctl addif br1 tap1
ip link set br1 up

ip addr add local 192.168.10.10/24 dev tap0
ip link set tap0 up
ip addr add local 192.168.10.11/24 dev tap1
ip link set tap1 up

