#!/bin/sh
# $Id: test1.sh,v 1.6 2004-09-27 10:44:51 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log

rm -f $LOG
if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters|grep grs.xml >/dev/null; then
        ../../index/zebraidx -c $pp/zebra.cfg -l$LOG init
else
        exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/f1.xml
