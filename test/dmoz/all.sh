#!/bin/sh
# $Id: all.sh,v 1.3 2002-06-19 09:00:28 adam Exp $
./update.sh b
./update.sh c
./update.sh d
gnuplot plot.dem
