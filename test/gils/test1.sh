#!/bin/sh

set -e
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

../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra1.cfg -l $LOG unix:socket

echo "checking it runs..." >>$LOG
test -f z.pid
. "$srcdir/cleanup.sh"

echo "search 1..." >>$LOG
../api/testclient -c 17 unix:socket utah >> $LOG

echo "search 2..." >>$LOG
../api/testclient -c 41 unix:socket "@or utah the" >> $LOG

echo "search 3..." >>$LOG
../api/testclient -c 1 unix:socket "@attr 1=4 the" >> $LOG

echo "search 4..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >> $LOG

echo "reindexing..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records

echo "search 5..." >>$LOG
../api/testclient -c 18 unix:socket "@attr 1=4 utah" >> $LOG

