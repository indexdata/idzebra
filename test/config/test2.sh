#!/bin/sh
if ../../index/zebraidx -V|grep Tcl >/dev/null; then
	../../index/zebraidx init
	../../index/zebraidx -s -t grs.tcl.m update m.rec | grep tag:dc:subject >/dev/null
else
	exit 0
fi
