#!/bin/sh
# $Id: all.sh,v 1.2 2002-06-19 08:32:34 adam Exp $
./update.sh b
./update.sh c
gnuplot plot.dem
