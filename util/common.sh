#!/bin/bash
#
# stops resources and deamons

function stop_gluster {
        /etc/init.d/glusterd start
        gluster volume geo-replication georep root@kvm.local:/media/physical stop
        yes | gluster volume stop georep force
        yes | gluster volume delete georep
        /etc/init.d/glusterd stop
}

function stop_syncedfs {
        killall syncedfs-daemon
}

function stop_drbd {
		drbdadm down r0
		drbdadm invalidate r0
}

#function stop_native {
#       # nothing for now
#}
