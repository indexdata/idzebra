#!/bin/sh
# $Id: test7.sh,v 1.1.2.1 2006-02-06 23:11:12 adam Exp $

pp=${srcdir:-"."}

LOG="test7.log"
TMP="test7.tmp"
DBG=""

rm -f $LOG
rm -f $TMP.*
../../index/zebraidx -c $pp/zebra6.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra6.cfg -l $LOG -t grs.xml update $pp/rec61.xml $pp/rec62.xml || exit 2
test -f dict*.mf || exit 1

../../index/zebrasrv -c $pp/zebra6.cfg -l $LOG $DBG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2

# bug 460
../api/testclient unix:socket '@attr 1=4 46' >$TMP.1

../api/testclient unix:socket '@attr 1=4 beta' >$TMP.2

echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
sleep 1
echo "Result counts: " >> $LOG
cat $TMP.* >>$LOG

echo 'Checking that result 1 count is 0' >>$LOG
grep "^Result count: 0$" $TMP.1 >/dev/null || exit 4

echo 'Checking that result 2 count is 1' >>$LOG
grep "^Result count: 1$" $TMP.2 >/dev/null || exit 5

echo 'Test OK' >>$LOG
exit 0

