#!/bin/bash

for f in $(ls test*); do
	./runtest.sh $f 1>/dev/null 2>/dev/null
	if [ $? -eq 0 ]; then
		echo "$f succeeded"
	else
		echo "$f failed"
	fi
done
