#!/bin/bash

# is the server running?
if ps -eo pid,args | grep "syncedfs-daemon $SECONDARYRES server" | grep -v grep > /dev/null; then
	:
else
	$DAEMONBIN $SECONDARYRES server
fi

$DAEMONBIN $PRIMARYRES client
