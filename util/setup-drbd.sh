#!/bin/bash
#
# /media/btrfs - btrfs
#       - physical (btrfs: inherited)
#       - virtual (btrfs: inherited)

function usage {
    echo "usage: setup-drbd.sh [master|slave]"
    exit 1
}

PART=/dev/drbd1


if [ $# -ne 1 ]; then
    usage
fi

# unmount possibly mounted partitions
umount /media/btrfs/virtual
umount /media/btrfs

source /root/common.sh

stop_drbd
stop_syncedfs
stop_gluster

# enable resourced
yes yes | drbdadm create-md r0
drbdadm up r0

if [ $1 = 'master' ]; then
	# trigger initial synchronization
	drbdadm primary --force r0

	# reformat partition
	mkfs.btrfs $PART

	# in DRBD we only create the directory structure on master node
	mkdir -p /media/btrfs
	mount -t btrfs $PART /media/btrfs

	mkdir -p /media/btrfs/physical
	mkdir -p /media/btrfs/virtual

	chmod 755 /media/btrfs
	chown -R lfr.lfr /media/btrfs
	
	# now wait really long for initalial sync to finish
else
    usage
fi
