#!/bin/sh
test -d tmp || mkdir tmp
test -d lock || mkdir lock
echo Loading Records
if [ -x ../../index/zmbolidx ]; then
	../../index/zmbolidx update records
fi
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx update records
fi
echo Starting Server
if [ -x ../../index/zmbolsrv ]; then
	../../index/zmbolsrv
fi
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
