#!/bin/bash
#
# /media
#       - physical (btrfs for gluster and syncedfs none for native and drbd)
#       - virtual (fuse.gluster, fuse.syncedfs or btrfs for native and drbd)

function usage {
	echo "usage: setup-glusterfs.sh [master|slave]"
	exit 1
}

PART=/dev/sda4


if [ $# -ne 1 ]; then
	usage
fi

source /root/common.sh

#stop_drbd
#stop_syncedfs
stop_gluster

# unmount possibly mounted partitions
umount /media/physical
umount /media/virtual

mkdir -p /media/physical
mkdir -p /media/virtual

# reformat partition
mkfs.btrfs $PART
mount -t btrfs /dev/sda4 /media/physical

if [ $1 = 'master' ]; then
	/etc/init.d/glusterd start
	# create gluster volume
	gluster volume create georep transport tcp xen.local:/media/physical
	gluster volume start georep
	gluster volume geo-replication georep root@kvm.local:/media/physical start
	mount -t glusterfs xen.local:/georep /media/virtual
elif [ $1 = 'slave' ]; then
	/etc/init.d/glusterd start
else
	usage
fi
