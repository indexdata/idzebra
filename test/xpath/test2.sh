#!/bin/sh
# $Id: test2.sh,v 1.3 2003-05-21 14:39:23 adam Exp $
LOG=test2.log
TMP=test2.tmp
rm -f $LOG
rm -f $TMP
../../index/zebraidx -l $LOG init || exit 1
../../index/zebraidx -l $LOG -t grs.sgml update rec.xml || exit 2
test -f dict*.mf || exit 1
../../index/zebrasrv -l $LOG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2
../api/testclient unix:socket '@attr 1=/Zthes/termName Sauropoda' >$TMP
echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
cat $TMP >>$LOG
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP >/dev/null || exit 4
echo 'Test OK' >>$LOG

