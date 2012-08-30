#!/bin/bash
#
# Generating data for assessing performance of distributed file systems.
#
COUNT=

function usage {
	echo "usage: fstest test-name count path [sourcefile]"
	exit 1
}

function create {
	if [ ! -d $2 ]; then
		echo Directory does not exist
		usage
	fi

	DIR=${2%/}
	DATE=$(date +%F_%k-%M-%S)
	for (( i=0 ; i<$COUNT ; i++ )); do
		echo $RANDOM:$DATE:$i > $DIR/$DATE.$i
	done
}

function delete {
	if [ ! -d $2 ]; then
		echo Directory does not exist
		usage
	fi
	
	DIR=${2%/}
	cnt=0
	for f in $(ls $DIR); do
		if [ $cnt -eq $COUNT ]; then
			break
		fi

		rm -f $DIR/$f 

		cnt=$((cnt+1))
	done
}

function convert_size {
	COUNT=${1%[kmgKMG]}
	SUFFIX=${1##*[0-9]}

	case "$SUFFIX" in
		k | K )
			COUNT=$(($COUNT * 1024))
			;;
		m | M )
			COUNT=$(($COUNT * 1024 * 1024))
			;;	
		g | G )
			COUNT=$(($COUNT * 1024 * 1024 * 1024))
			;;	
		* )
			COUNT=$1
	esac
}

USER=lfr

if [ $# -lt 3 ]; then
	usage
	exit 1
fi

convert_size $2

if [ -n "$1" ]; then
	case "$1" in
		seqwrite )
			fswrite64 seqwrite $COUNT $4 $3
			;;
		ranwrite )
			fswrite64 ranwrite $COUNT $4 $3
			;;
		append )
			fswrite64 append $COUNT $4 $3
			;;
		truncate )
			truncate -s $COUNT $3
			;;
		read )
			cmp $4 $3
			;;
		create )
			create $COUNT $3
			;;
		delete )
			delete $COUNT $3
			;;
		* ) 
			usage
			;;
	esac
fi
