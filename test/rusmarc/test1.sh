#!/bin/sh
# $Id: test1.sh,v 1.2 2004-03-09 18:52:37 adam Exp $
LOG=test1.log
rm -f $LOG
test -d tmp || mkdir tmp
test -d lock || mkdir lock
test -d register || mkdir register
../../index/zebraidx -l$LOG init
../../index/zebraidx -l$LOG update records/*marc
../../index/zebrasrv -l$LOG unix:socket &
sleep 1
# search text located in first record 600 $a
# term is koi8-r encoded
../api/testclient unix:socket '@attr 1=21 Замятин' >tmp1
echo 'Result count: 1' >tmp2
kill `cat lock/zebrasrv.pid` || exit 1
diff tmp1 tmp2 || exit 2
rm -f tmp1 tmp2
