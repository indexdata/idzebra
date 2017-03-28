#!/bin/sh
# tests that updates are reflected immediately # in the registers.
# Repeatedly modifies a record and counts hits.
# Test 1: with good sleeps in every between - should pass always

srcdir=${srcdir:-"."}

if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=timing1.log

rm -f $LOG

echo "  init..." >>$LOG

# these should not be here, will be created later
$srcdir/cleanrecords.sh


../../index/zebraidx -c $srcdir/zebra2.cfg -l $LOG init || exit 1

echo "  starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra2.cfg -l $LOG unix:socket
test -f z.pid || exit 1
sleep 1

echo "  update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1
sleep 1

echo "  search 1..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1
sleep 1

echo "making a test record..." >>$LOG
cp $srcdir/records/esdd0006.grs $srcdir/records/esdd0002.grs

echo "  indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1
sleep 1

echo "  search 2..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 1
echo "  1a: modifying a test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 3..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 1
echo "  1b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 4..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 1
echo "  2a: modifying the test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 5..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 1
echo "  2b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

sleep 1
echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 6..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 1

echo "  3a: modifying the test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

sleep 1
echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 7..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 1
echo "  3b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records || exit 1

sleep 1
echo "    search 8..." >>$LOG
../api/testclient unix:socket "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f log
$srcdir/cleanrecords.sh
rm -f z.pid

