#!/bin/sh
# $Id: test2.sh,v 1.3 2004-06-15 08:06:34 adam Exp $

pp=${srcdir:-"."}

LOG=test2.log

if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG -V|grep Tcl >/dev/null; then
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG -s -t grs.tcl.m update m.rec | grep tag:dc:subject >/dev/null
else
	exit 0
fi
