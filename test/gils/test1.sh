#!/bin/sh
# $Id: test1.sh,v 1.9 2004-06-15 09:43:30 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log

rm -f $LOG
echo  "initializing..." >>$LOG
test -d reg || mkdir reg
rm -f $pp/records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update $pp/records  || exit 1

echo "killing old server (if any)..." >>$LOG
test -f zebrasrv.pid && kill `cat zebrasrv.pid`

echo "starting server..." >>$LOG
../../index/zebrasrv -S -c $pp/zebra1.cfg -l $LOG tcp:@:9901 &
sleep 1

echo "checking it runs..." >>$LOG
test -f zebrasrv.pid || exit 1

echo "search 1..." >>$LOG
../api/testclient localhost:9901 utah > log || exit 1
grep "^Result count: 17$" log >/dev/null || exit 1

echo "search 2..." >>$LOG
../api/testclient localhost:9901 "@or utah the" > log || exit 1
grep "^Result count: 40$" log >/dev/null || exit 1

echo "search 3..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 the" > log || exit 1
grep "^Result count: 1$" log >/dev/null || exit 1

echo "search 4..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "reindexing..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update $pp/records || exit 1

echo "search 5..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 18$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f zebrasrv.pid

