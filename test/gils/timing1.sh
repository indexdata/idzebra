#!/bin/sh
# $Id: timing1.sh,v 1.9 2004-06-15 09:43:30 adam Exp $
# tests that updates are reflected immediately # in the registers.
# Repeatedly modifies a record and counts hits.
# Test 1: with good sleeps in every between - should pass always

pp=${srcdir:-"."}

LOG=timing1.log

rm -f $LOG

echo "  init..." >>$LOG
rm -f $pp/records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -c $pp/zebra2.cfg -l $LOG init || exit 1

echo "  killing old server (if any)..." >>$LOG
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`

echo "  starting server..." >>$LOG
../../index/zebrasrv -S -c $pp/zebra2.cfg -l $LOG tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1
sleep 2

echo "  update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1
sleep 2

echo "  search 1..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1
sleep 2

echo "making a test record..." >>$LOG
cp $pp/records/esdd0006.grs $pp/records/esdd0002.grs

echo "  indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1
sleep 2

echo "  search 2..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 2
echo "  1a: modifying a test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 3..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 2
echo "  1b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 4..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 2
echo "  2a: modifying the test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 5..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 2
echo "  2b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

sleep 2
echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 6..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

sleep 2

echo "  3a: modifying the test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

sleep 2
echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 7..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

sleep 2
echo "  3b: modifying the test record back (utah)..." >>$LOG
sed 's/XYZ/UTAH/g' <$pp/records/esdd0002.grs >$pp/records/esdd0002x.grs
mv $pp/records/esdd0002x.grs $pp/records/esdd0002.grs

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update $pp/records || exit 1

sleep 2
echo "    search 8..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f log
rm -f $pp/records/esdd000[12].grs 
rm -f zebrasrv.pid

