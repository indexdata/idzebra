#!/bin/sh
# $Id: stop03.sh,v 1.4 2003-05-21 14:39:22 adam Exp $
# test start and stop of the threaded server (-T)
#
# FIXME - this test does not currently pass  (H 22-oct-2002)
# When the client exits, the process dies with a segfault

echo "initializing"
mkdir -p reg
rm -f idx.log srv.log servercrash
../../index/zebraidx -l idx.log -c zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "Starting server with -T (threaded)..."
(
  ../../index/zebrasrv -T -c zebra1.cfg -l srv.log tcp:@:9901 ||
    echo "server failed with $?" > servercrash
)&
sleep 1

echo "  checking that it runs... "
test -f zebrasrv.pid || exit 1
PID=`cat zebrasrv.pid`
ps -p $PID | grep $PID >/dev/null || exit 1

echo "  connecting to it..."
../api/testclient localhost:9901 utah > log || exit 1
sleep 1

echo "  checking that it still runs..."
ps -p $PID >/dev/null || exit 1

echo "  connecting again, with a delay..."
../api/testclient localhost:9901 utah 5 > log &
sleep 1 # let the client connect 

echo "  killing it..."
kill  $PID
sleep 1

echo "  checking that it is dead"
ps -p $PID | grep $PID >/dev/null && exit 1

# clean up
rm -rf reg idx.log srv.log zebrasrv.pid servercrash
