#!/bin/sh
# $Id: bench1.sh,v 1.1 2006-12-09 08:03:57 adam Exp $
# Test for variations of number of trees.. Total number of entries is
# Constant 1 mio
CMD="./benchisamb -r 50"

$CMD -i 1000 -n 1000 >1000x1000.dat
sleep 2
$CMD -i 100 -n 10000 >100x10000.dat
sleep 2
$CMD -i 10 -n 100000 >10x100000.dat
sleep 2
$CMD -i 1 -n 1000000 >1x1000000.dat
