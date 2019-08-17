#!/bin/bash
export PATH=$PATH:/sd/bridge-utils-1.6/build/sbin
echo "----------brctl show--------------"
brctl show
echo "----------brctl addbr br0------------"
brctl addbr br0
echo "----------brctl show--------------"
brctl show

echo "brctl addif br0 eth0/1"
brctl addif br0 eth0
brctl addif br0 eth1
echo "----------brctl show--------------"
brctl show

echo "----ip link set br0 up--------------"
ip link set br0 up
