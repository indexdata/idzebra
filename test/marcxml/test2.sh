#!/bin/sh
LOG=test2.log
rm -f $LOG
../../index/zebraidx -l$LOG init
../../index/zebraidx -l$LOG -t grs.marcxml.record update sample-marc
../../index/zebrasrv -l$LOG unix:socket &
sleep 1
../api/testclient unix:socket '@and @attr 1=1003 jack @attr 1=4 computer' >tmp1
echo 'Result count: 2' >tmp2
kill `cat zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
