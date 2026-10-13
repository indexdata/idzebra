#!/bin/sh

set -e
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

../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg init

echo "update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "update 2..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra2.cfg -l $LOG unix:socket
test -f z.pid
. "$srcdir/cleanup.sh"

echo "search 1..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >> $LOG

echo "update 3..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "search 2..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >> $LOG

echo "making test records..." >>$LOG
cp $srcdir/records/esdd0006.grs $srcdir/records/esdd0001.grs

echo "indexing them..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "search 3..." >>$LOG
../api/testclient -c 10 unix:socket "@attr 1=4 utah" >> $LOG
touch $srcdir/records/esdd0001.grs

echo "indexing again..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "search 4..." >>$LOG
../api/testclient -c 10 unix:socket "@attr 1=4 utah" >> $LOG

echo "making another test record..." >>$LOG
mv $srcdir/records/esdd0001.grs $srcdir/records/esdd0002.grs

echo "indexing it too..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "search 5..." >>$LOG
../api/testclient -c 10 unix:socket "@attr 1=4 utah" >> $LOG

sleep 1
echo "modifying a test record..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records
sleep 1

echo "search 6..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >> $LOG

echo "search 7..." >>$LOG
../api/testclient -c 1 unix:socket "@attr 1=4 xyz" >> $LOG
