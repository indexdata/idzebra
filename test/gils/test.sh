#!/bin/sh
if [ -x ../../index/zmbolidx ]; then
	IDX=../../index/zmbolidx
	SRV=../../index/zmbolsrv
elif [ -x ../../index/zebraidx ]; then 
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
