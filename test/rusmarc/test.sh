#!/bin/sh
# $Id: test.sh,v 1.5 2004-12-04 00:51:39 adam Exp $

echo Loading Records
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx update records/simple-rusmarc
fi
echo Starting Server
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
