#!/bin/sh
LOG=test2.log
rm -f $LOG
if ../../index/zebraidx -l $LOG -V|grep Expat >/dev/null; then
	../../index/zebraidx -l$LOG init
else
	exit 0
fi
../../index/zebraidx -l$LOG update *.xml
../../index/zebrasrv -l$LOG unix:socket &
sleep 1
# search for UNICODE 1E25 - letter h with dot below
../api/testclient unix:socket '@term string ḥ' >tmp1
echo 'Result count: 1' >tmp2
kill `cat zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
