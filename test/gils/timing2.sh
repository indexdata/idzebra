#!/bin/sh
# $Id: timing2.sh,v 1.6 2003-05-21 14:39:22 adam Exp $ 
# Demonstrated that updates depend on file timestamps

echo "Testing timings of updates"
echo "  init..."
rm -f idx.log log timeref[12]
rm -f records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -c zebra2.cfg -l idx.log init || exit 1
touch timeref1  # make an early timestamp

echo "  killing old server (if any)..."
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "  starting server..."
../../index/zebrasrv -S -c zebra2.cfg -l srv.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1
touch timeref2  # make a later timestamp

echo "  update 1..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "  search 1..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "making a test record..."
cp records/esdd0006.grs records/esdd0002.grs
touch -r timeref1 records/esdd0002.grs

echo "  indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "  search 2..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1

echo "  modifying a test record (xyz)..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs
touch -r timeref1 records/esdd0002.grs # reset timestamp to 'early'

echo "    not indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 3..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 10$" log || exit 1

echo "  touching its timestamp..."
touch -r timeref2 records/esdd0002.grs # set timestamp to 'late'

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 4..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 9$" log || exit 1

echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f idx.log log timeref[12]
rm -f records/esdd000[12].grs 
rm -f zebrasrv.pid
rm -f srv.log

