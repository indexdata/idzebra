#!/bin/sh
LOG=test1.log
rm -f $LOG
if ../../index/zebraidx -l $LOG -V|grep Expat >/dev/null; then
        ../../index/zebraidx -l$LOG init
else
        exit 0
fi
../../index/zebraidx -l $LOG update f1.xml
