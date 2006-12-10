#!/bin/sh
# $Id: bench2.sh,v 1.1 2006-12-10 11:54:29 adam Exp $
# Test for variations in size of ISAM entry
CMD="./benchisamb -r 50 -n 1000000 -i 1"

for sz in 0 4 8 12; do
	$CMD -z $sz >bench2.$sz.dat
	ls -l *.mf
	sleep 2
done

