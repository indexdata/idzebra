#!/bin/sh
# $Id: test2.sh,v 1.14 2006-04-27 10:52:26 marc Exp $

srcdir=${srcdir:-"."}

if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=test2.log

rm -f $LOG
echo "init..." >>$LOG

# these should not be here, will be created later
$srcdir/cleanrecords.sh

../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg init || exit 1

echo "update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "update 2..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "killing old server (if any)..." >>$LOG
test -f z.pid && kill -9 `cat z.pid`

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra2.cfg -l $LOG unix:socket
test -f z.pid || exit 1

echo "search 1..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "update 3..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "search 2..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "making test records..." >>$LOG
cp $srcdir/records/esdd0006.grs $srcdir/records/esdd0001.grs

echo "indexing them..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "search 3..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1
touch $srcdir/records/esdd0001.grs

echo "indexing again..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "search 4..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

echo "making another test record..." >>$LOG
mv $srcdir/records/esdd0001.grs $srcdir/records/esdd0002.grs

echo "indexing it too..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

echo "search 5..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 1
echo "modifying a test record..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1
sleep 1

echo "search 6..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "search 7..." >>$LOG
../api/testclient unix:socket "@attr 1=4 xyz" > log || exit 1
grep "^Result count: 1$" log >/dev/null || exit 1

echo "stosrcdiring server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f log
$srcdir/cleanrecords.sh
rm -f z.pid
