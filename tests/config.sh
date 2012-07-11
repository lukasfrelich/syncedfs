#!/bin/bash

if [ $# -eq 1 ] && [ $1 = 'unset' ]; then
	unset WRITEDIR
	unset CMPDIRPRIMARY
	unset CMPDIRSECONDARY
	unset DAEMONBIN
	unset PRIMARYRES
	unset SECONDARYRES
else
	# needed for write
	export WRITEDIR='/media/btrfs/syncedfs/primary/virtual/test'
	
	# needed for diff
	export CMPDIRPRIMARY='/media/btrfs/syncedfs/primary/physical/test'
	export CMPDIRSECONDARY='/media/btrfs/syncedfs/secondary/physical/test'
	
	# needed for syncedfs-daemon
	export DAEMONBIN='/home/lfr/workspace/fsperf/syncedfs-daemon/dist/Debug/GNU-Linux-x86/syncedfs-daemon'
	export PRIMARYRES='r0'
	export SECONDARYRES='r0s'
fi
