#!/bin/sh
# $Id: test2.sh,v 1.6 2003-05-21 14:39:22 adam Exp $
echo "Testing with storekeys (zebra2.cfg)"
echo "init..."
rm -f idx.log log
rm -f records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -l idx.log -c zebra2.cfg init || exit 1

echo "update 1..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "update 2..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "killing old server (if any)..."
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "starting server..."
../../index/zebrasrv -S -c zebra2.cfg -l srv.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1

echo "search 1..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "update 3..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "search 2..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "making test records..."
cp records/esdd0006.grs records/esdd0001.grs

echo "indexing them..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "search 3..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1
touch records/esdd0001.grs

echo "indexing again..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "search 4..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1

echo "making another test record..."
mv records/esdd0001.grs records/esdd0002.grs

echo "indexing it too..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "search 5..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1

sleep 1
echo "modifying a test record..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1
sleep 1

echo "search 6..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "search 7..."
../api/testclient localhost:9901 "@attr 1=4 xyz" > log || exit 1
grep "^Result count: 1$" log || exit 1

echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f idx.log log
rm -f records/esdd000[12].grs 
rm -f zebrasrv.pid
rm -f srv.log
