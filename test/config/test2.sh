#!/bin/sh

pp=${srcdir:-"."}

LOG=test2.log

if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters|grep grs.tcl >/dev/null; then
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG -s -t grs.tcl.m update $pp/m.rec | grep tag:dc:subject >/dev/null
else
	exit 0
fi
