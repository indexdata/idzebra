#!/bin/sh
# $Id: stop01.sh,v 1.4 2003-05-21 14:39:22 adam Exp $
# test start and stop of the server with -1

echo "initializing"
mkdir -p reg
rm -f idx.log srv.log
../../index/zebraidx -l idx.log -c zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "Starting server with -1 (one shot)..."
../../index/zebrasrv -1 -c zebra1.cfg -l srv.log tcp:@:9901 &
sleep 1

echo "  checking that it runs... "
test -f zebrasrv.pid || exit 1
PID=`cat zebrasrv.pid`
ps -p $PID |grep $PID >/dev/null || exit 1

echo "  connecting to it..."
../api/testclient localhost:9901 utah > log || exit 1
sleep 1

echo "  checking that server does not run any more"
ps -p $PID | grep $PID >/dev/null && exit 1

# clean up
rm -rf reg idx.log srv.log zebrasrv.pid
