#!/bin/sh
# $Id: test1.sh,v 1.5 2004-09-27 11:22:39 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
rm -f $LOG
if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters|grep grs.xml >/dev/null; then
	../../index/zebraidx -c $pp/zebra.cfg -l$LOG init
else
	exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l$LOG update $pp/*.xml
../../index/zebrasrv -c $pp/zebra.cfg -l$LOG unix:socket &
sleep 2
../api/testclient unix:socket '@term string æ' >tmp1
echo 'Result count: 1' >tmp2
kill `cat zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
