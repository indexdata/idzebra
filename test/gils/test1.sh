#!/bin/sh
# $Id: test1.sh,v 1.13 2005-01-03 12:08:04 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log

rm -f $LOG
echo  "initializing..." >>$LOG
test -d reg || mkdir reg
rm -f $pp/records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update $pp/records  || exit 1

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $pp/zebra1.cfg -l $LOG unix:socket

echo "checking it runs..." >>$LOG
test -f z.pid || exit 1

echo "search 1..." >>$LOG
../api/testclient unix:socket utah > log || exit 1
grep "^Result count: 17$" log >/dev/null || exit 1

echo "search 2..." >>$LOG
../api/testclient unix:socket "@or utah the" > log || exit 1
grep "^Result count: 40$" log >/dev/null || exit 1

echo "search 3..." >>$LOG
../api/testclient unix:socket "@attr 1=4 the" > log || exit 1
grep "^Result count: 1$" log >/dev/null || exit 1

echo "search 4..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "reindexing..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update $pp/records || exit 1

echo "search 5..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 18$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f z.pid

