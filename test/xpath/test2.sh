#!/bin/sh
LOG=test2.log
TMP=test2.tmp
rm -f $LOG
test -f dict*.mf || exit 1
../../index/zebrasrv -l $LOG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2
../testclient unix:socket '@attr 1=/Zthes/termName Sauropoda' >$TMP
echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
cat $TMP >>$LOG
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP || exit 4
echo 'Test OK' >>$LOG

