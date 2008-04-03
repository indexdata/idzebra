#!/bin/sh

pp=${srcdir:-"."}

LOG=test1.log

rm -f $LOG
if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters|grep grs.xml >/dev/null; then
        ../../index/zebraidx -c $pp/zebra.cfg -l$LOG init
else
        exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/f1.xml
