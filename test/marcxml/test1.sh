#!/bin/sh
# $Id: test1.sh,v 1.4.2.2 2006-02-06 13:19:55 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
DBG="-v 1647"

rm -f $LOG
if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG -V|grep Expat >/dev/null; then
	../../index/zebraidx -c $pp/zebra.cfg -l $LOG $DBG init
else
	exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/m*.xml
../../index/zebrasrv -c $pp/zebra.cfg -l $LOG unix:socket &
ret=0
sleep 1
../api/testclient unix:socket '@and @attr 1=54 eng @and @attr 1=1003 jack @attr 1=4 computer' >tmp1
echo 'Result count: 2' >tmp2
diff tmp1 tmp2 || ret=1
rm -f tmp1 tmp2
# bug # 460
../api/testclient unix:socket '@attr 1=1003 a' >tmp1
echo 'Result count: 0' >tmp2
diff tmp1 tmp2 || ret=1
rm -f tmp1 tmp2
kill `cat zebrasrv.pid` || exit 1
exit $ret
