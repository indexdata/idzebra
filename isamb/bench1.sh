#!/bin/sh
# Test for variations of number of trees.. Total number of entries is
# Constant 1 mio
CMD="./benchisamb -r 50"

$CMD -i 1000 -n 1000 >bench1.1000.dat
ls -l *.mf
sleep 2
$CMD -i 100 -n 10000 >bench1.100.dat
ls -l *.mf
sleep 2
$CMD -i 10 -n 100000 >bench1.10.dat
ls -l *.mf
sleep 2
$CMD -i 1 -n 1000000 >bench1.1.dat
ls -l *.mf
