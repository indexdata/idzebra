#!/bin/sh
# $Id: test2.sh,v 1.9 2004-06-15 09:43:30 adam Exp $

pp=${srcdir:-"."}

LOG=test2.log

rm -f $LOG
echo "init..." >>$LOG
rm -f $pp/records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg init || exit 1

echo "update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "update 2..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "killing old server (if any)..." >>$LOG
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`

echo "starting server..." >>$LOG
../../index/zebrasrv -S -c $pp/zebra2.cfg -l $LOG tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1

echo "search 1..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "update 3..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "search 2..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "making test records..." >>$LOG
cp $pp/records/esdd0006.grs $pp/records/esdd0001.grs

echo "indexing them..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "search 3..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1
touch $pp/records/esdd0001.grs

echo "indexing again..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "search 4..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

echo "making another test record..." >>$LOG
mv $pp/records/esdd0001.grs $pp/records/esdd0002.grs

echo "indexing it too..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

echo "search 5..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 1
echo "modifying a test record..." >>$LOG
sed 's/UTAH/XYZ/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

echo "indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1
sleep 1

echo "search 6..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "search 7..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 xyz" > log || exit 1
grep "^Result count: 1$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f log
rm -f $pp/records/esdd000[12].grs 
rm -f zebrasrv.pid
