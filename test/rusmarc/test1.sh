#!/bin/sh
# $Id: test1.sh,v 1.6 2004-09-27 10:44:52 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
rm -f $LOG
test -d tmp || mkdir tmp
test -d lock || mkdir lock
test -d register || mkdir register

if ../../index/zebraidx -c $pp/zebra.cfg -l $LOG filters|grep grs.marc >/dev/null; then
        ../../index/zebraidx -c $pp/zebra.cfg -l$LOG init
else
        exit 0
fi

if grep 'UTF-8 to koi8-r unsupported' $LOG >/dev/null; then
	exit 0
fi
../../index/zebraidx -c $pp/zebra.cfg -l$LOG update $pp/records/*marc
../../index/zebrasrv -c $pp/zebra.cfg -l$LOG unix:socket &
sleep 1
# search text located in first record 600 $a
# term is koi8-r encoded
../api/testclient unix:socket '@attr 1=21 Замятин' >tmp1
echo 'Result count: 1' >tmp2
kill `cat lock/zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
