#!/bin/sh

echo Loading Records
if [ -x ../../index/zebraidx ]; then
	../../index/zebraidx update records/simple-rusmarc
fi
echo Starting Server
if [ -x ../../index/zebrasrv ]; then
	../../index/zebrasrv
fi
