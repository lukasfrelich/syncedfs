#!/bin/bash
#
# /media/btrfs
#       - physical (btrfs: inherited)
#       - virtual (fuse.syncedfs)

function usage {
    echo "usage: setup-syncedfs.sh [master|slave]"
    exit 1
}

PART=/dev/sda4


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

# reformat partition
mkfs.btrfs $PART

mkdir -p /media/btrfs
mount -t btrfs /dev/sda4 /media/btrfs

# create directories for pid files
mkdir -p /var/run/syncedfs/fs
mkdir -p /var/run/syncedfs/client
mkdir -p /var/run/syncedfs/server
chown -R lfr.lfr /var/run/syncedfs

# create necessary subvolumes
btrfs subvolume create /media/btrfs/physical
mkdir -p /media/btrfs/virtual
chmod 755 /media/btrfs
chown -R lfr.lfr /media/btrfs

if [ $1 = 'master' ]; then
	sudo -u lfr syncedfs r0
elif [ $1 = 'slave' ]; then
	sudo -u lfr syncedfs-daemon r0 server
else
    usage
fi
