#!/bin/sh
# $Id: test.sh,v 1.8 2003-05-06 17:39:01 adam Exp $
if [ -x ../../index/zebraidx ]; then 
	IDX=../../index/zebraidx
	SRV=../../index/zebrasrv
else
	echo "No indexer found"
	exit 1
fi
echo Loading Records
$IDX -t grs.sgml update records
echo Starting Server
$SRV
