#!/bin/sh
rm zebraidx.log
./update.sh b
./update.sh c
gnuplot plot.dem
