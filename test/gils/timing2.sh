#!/bin/sh
# $Id: timing2.sh,v 1.8 2004-06-15 08:06:34 adam Exp $ 
# Demonstrated that updates depend on file timestamps

pp=${srcdir:-"."}

LOG=timing2.log

rm -f $LOG

echo "  init..." >>$LOG
rm -f log timeref[12]
rm -f records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -c $pp/zebra2.cfg -l $LOG init || exit 1
touch timeref1  # make an early timestamp

echo "  killing old server (if any)..." >>$LOG
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`

echo "  starting server..." >>$LOG
../../index/zebrasrv -S -c $pp/zebra2.cfg -l $LOG tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1
touch timeref2  # make a later timestamp

echo "  update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update records || exit 1

echo "  search 1..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log >/dev/null || exit 1

echo "making a test record..." >>$LOG
cp records/esdd0006.grs records/esdd0002.grs
touch -r timeref1 records/esdd0002.grs

echo "  indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update records || exit 1

echo "  search 2..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log >/dev/null || exit 1

echo "  modifying a test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs
touch -r timeref1 records/esdd0002.grs # reset timestamp to 'early'

echo "    not indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update records || exit 1

echo "    search 3..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 10$" log >/dev/null || exit 1

echo "  touching its timestamp..." >>$LOG
touch -r timeref2 records/esdd0002.grs # set timestamp to 'late'

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra2.cfg update records || exit 1

echo "    search 4..." >>$LOG
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..." >>$LOG
grep "^Result count: 9$" log >/dev/null || exit 1

echo "stopping server..." >>$LOG
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f log timeref[12]
rm -f records/esdd000[12].grs 
rm -f zebrasrv.pid

