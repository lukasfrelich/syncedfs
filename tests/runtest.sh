#!/bin/bash

function usage {
  	echo "usage: runtest.sh [-n] test-name"
	echo "       -n   nocleanup"
    exit 1
}

function sync {
	DIR=$(pwd)

	cd $SCRIPTDIR
	source sync.sh

	cd $DIR
}

SCRIPTDIR=$(pwd)
TEST=

# parameters processing
if [ $# -eq 1 ]; then
	TEST=$1
elif [ $# -eq 2 ] && [ $1 = '-n' ]; then
	NOCLEANUP='true'
	TEST=$2
else
	usage
fi

# config + setup common to all tests
source config.sh
source setup.sh

# run test
if [ $TEST = 'cleanup' ]; then
	source cleanup.sh
	exit
else
	source $TEST
fi

sync

# compare trees
cd $SCRIPTDIR
source cmptrees.sh

# cleanup (if desired)
RET=$?
if [ $RET -eq 0 ]; then
	echo "Test run successfully."
	if [ -z $NOCLEANUP ]; then
		echo "Cleaning up."
		source cleanup.sh
	fi
else
	echo "Test failed."
fi
source config.sh unset
exit $RET
