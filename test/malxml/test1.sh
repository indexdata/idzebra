#!/bin/sh
# $Id: test1.sh,v 1.5 2004-06-15 09:43:32 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log

rm -f $LOG
if ../../index/zebraidx -l $LOG -V|grep Expat >/dev/null; then
        ../../index/zebraidx -c $pp/zebra.cfg -l$LOG init
else
        exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/f1.xml
