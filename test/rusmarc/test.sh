#!/bin/sh
# $Id: test.sh,v 1.4 2004-03-09 18:45:17 adam Exp $
test -d tmp || mkdir tmp
test -d lock || mkdir lock
test -d register || mkdir register

echo Loading Records
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx update records/simple-rusmarc
fi
echo Starting Server
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
