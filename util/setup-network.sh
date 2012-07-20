#!/bin/bash
#
# reset to default
tc qdisc del dev eth2 root

# combination of two netem branches and htb rate limit
tc qdisc add dev eth2 root handle 1: htb default 12
tc class add dev eth2 parent 1:1 classid 1:12 htb rate 10Mbit ceil 10Mbit
tc qdisc add dev eth2 parent 1:12 handle 10:0 netem loss 20%
tc qdisc add dev eth2 parent 10:1 netem delay 1000ms

# orig
#tc qdisc add dev eth2 root handle 1: htb default 12
#tc class add dev eth2 parent 1:1 classid 1:12 htb rate 10Mbit ceil 10Mbit
#tc qdisc add dev eth2 parent 1:12 netem delay 1000ms
