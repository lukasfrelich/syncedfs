#!/bin/bash
#
# /media/btrfs
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

mkdir -p /media/btrfs/physical
mkdir -p /media/btrfs/virtual
chmod 755 /media/btrfs
chown -R lfr.lfr /media/btrfs

if [ $1 = 'master' ]; then
	/etc/init.d/glusterd start
	# create gluster volume
	gluster volume create georep transport tcp xen.local:/media/btrfs/physical
	gluster volume start georep
	gluster volume geo-replication georep root@kvm.local:/media/btrfs/physical start
	mount -t glusterfs xen.local:/georep /media/btrfs/virtual
elif [ $1 = 'slave' ]; then
	/etc/init.d/glusterd start
else
	usage
fi
