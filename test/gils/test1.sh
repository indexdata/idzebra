#!/bin/sh
# $Id: test1.sh,v 1.6 2003-05-21 14:39:22 adam Exp $
echo "testing without stored keys (zebra1.cfg)"

sleep 1

echo  "initializing..."
mkdir -p reg
rm -f records/esdd000[12].grs # these should not be here, will be created later
rm -f idx.log srv.log
../../index/zebraidx -l idx.log -c zebra1.cfg init || exit 1

echo "updating..."
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 1

echo "killing old server (if any)..."
test -f zebrasrv.pid && kill `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "starting server..."
../../index/zebrasrv -S -c zebra1.cfg -l srv.log tcp:@:9901 &
sleep 1

echo "checking it runs..."
test -f zebrasrv.pid || exit 1

echo "search 1..."
../api/testclient localhost:9901 utah > log || exit 1
grep "^Result count: 17$" log || exit 1

echo "search 2..."
../api/testclient localhost:9901 "@or utah the" > log || exit 1
grep "^Result count: 40$" log || exit 1

echo "search 3..."
../api/testclient localhost:9901 "@attr 1=4 the" > log || exit 1
grep "^Result count: 1$" log || exit 1

echo "search 4..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "reindexing..."
../../index/zebraidx -l idx.log -c  zebra1.cfg update records || exit 1

echo "search 5..."
../api/testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 18$" log || exit 1

echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 1
rm -f zebrasrv.pid

