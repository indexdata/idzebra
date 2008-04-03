#!/bin/sh

srcdir=${srcdir:-"."}

if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi


LOG=test1.log

rm -f $LOG
echo  "initializing..." >>$LOG
test -d reg || mkdir reg

# these should not be here, will be created later
$srcdir/cleanrecords.sh
#if [ -f $srcdir/records/esdd0001.grs ] 
#    then
#    rm -f $srcdir/records/esdd0001.grs
#fi
#if [ -f $srcdir/records/esdd0002.grs ] 
#    then
#    rm -f $srcdir/records/esdd0002.grs
#fi

../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init || exit 1

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records  || exit 1

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra1.cfg -l $LOG unix:socket

echo "checking it runs..." >>$LOG
test -f z.pid || exit 1

echo "search 1..." >>$LOG
../api/testclient unix:socket utah > log || exit 1
grep "^Result count: 17$" log >/dev/null || exit 1

echo "search 2..." >>$LOG
../api/testclient unix:socket "@or utah the" > log || exit 1
grep "^Result count: 41$" log >/dev/null || exit 1

echo "search 3..." >>$LOG
../api/testclient unix:socket "@attr 1=4 the" > log || exit 1
grep "^Result count: 1$" log >/dev/null || exit 1

echo "search 4..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "reindexing..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records || exit 1

echo "search 5..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 18$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f z.pid

