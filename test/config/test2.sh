#!/bin/sh
LOG=test2.log
if ../../index/zebraidx -l $LOG -V|grep Tcl >/dev/null; then
	../../index/zebraidx -l $LOG init
	../../index/zebraidx -l $LOG -s -t grs.tcl.m update m.rec | grep tag:dc:subject >/dev/null
else
	exit 0
fi
