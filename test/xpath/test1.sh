#!/bin/sh
rm -f test1.log
../../index/zebraidx -l test1.log init || exit 1
../../index/zebraidx -l test1.log -t grs.sgml update rec || exit 2
