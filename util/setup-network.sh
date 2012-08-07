#!/bin/bash
#
# setup network environment

function usage {
    echo "usage: setup-network.sh [reset | rate(Mbits) delay(ms) packet-loss(%)]"
    exit 1
}

function reset {
	tc qdisc del dev $DEV root
}

DEV=eth2

if [ $# -eq 1 -a $1 = 'reset' ]; then
	reset
	exit $?
fi

if [ $# -ne 3 ]; then
	usage
fi

RATE=$1
DELAY=$2
LOSS=$3

# reset to default
reset

# combination of two netem branches and htb rate limit
tc qdisc add dev $DEV root handle 1: htb default 12
tc class add dev $DEV parent 1:1 classid 1:12 htb rate ${RATE}Mbit ceil ${RATE}Mbit
tc qdisc add dev $DEV parent 1:12 handle 10:0 netem loss ${LOSS}%
tc qdisc add dev $DEV parent 10:1 netem delay ${DELAY}ms

# orig
#tc qdisc add dev $DEV root handle 1: htb default 12
#tc class add dev $DEV parent 1:1 classid 1:12 htb rate 10Mbit ceil 10Mbit
#tc qdisc add dev $DEV parent 1:12 netem delay 1000ms
