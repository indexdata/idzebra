#!/bin/sh
LOG=test1.log
rm -f $LOG
if ../../index/zebraidx -l $LOG -V|grep Expat >/dev/null; then
	../../index/zebraidx -l$LOG init
else
	exit 0
fi
../../index/zebraidx -l$LOG update m*.xml
../../index/zebrasrv -l$LOG unix:socket &
sleep 1
../api/testclient unix:socket '@and @attr 1=1003 jack @attr 1=4 computer' >tmp1
echo 'Result count: 2' >tmp2
kill `cat zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
