#!/bin/sh
# $Id: test.sh,v 1.3 2003-05-06 17:39:01 adam Exp $
test -d tmp || mkdir tmp
test -d lock || mkdir lock
test -d register || mkdir register

echo Loading Records
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx update records
fi
echo Starting Server
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
