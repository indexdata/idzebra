#!/bin/sh

set -e

pp=${srcdir:-"."}

LOG=test2.log

filters=$(../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters)
if printf '%s\n' "$filters" | grep grs.tcl >/dev/null; then
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
	output=$(../../index/zebraidx -c $pp/zebra.cfg -l $LOG -s -t grs.tcl.m update $pp/m.rec)
	printf '%s\n' "$output" | grep tag:dc:subject >/dev/null
else
	exit 0
fi
