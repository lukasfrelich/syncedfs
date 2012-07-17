#!/bin/bash

function stop_gluster {
        /etc/init.d/glusterd start
        gluster volume geo-replication georep root@kvm.local:/media/physical stop
        yes | gluster volume stop georep force
        yes | gluster volume delete georep
        /etc/init.d/glusterd stop
}

function stop_syncedfs {
        fusermount -u /media/virtual
        killall syncedfs-daemon
}

#function stop_drbd {
#
#}

#function stop_native {
#       # nothing for now
#}
