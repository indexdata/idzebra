#!/bin/sh
# $Id: stop03.sh,v 1.5 2003-05-24 22:34:48 adam Exp $
# test start and stop of the threaded server (-T)

LOG=stop03.log

rm -f $LOG
echo "initializing" >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`

echo "Starting server with -T (threaded)..." >>$LOG
(
  ../../index/zebrasrv -T -c zebra1.cfg -l $LOG tcp:@:9901 ||
    echo "server failed with $?" > $LOG
)&
sleep 1

echo "  checking that it runs... " >>$LOG
test -f zebrasrv.pid || exit 1
PID=`cat zebrasrv.pid`
ps -p $PID | grep $PID >/dev/null || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient localhost:9901 utah >>$LOG || exit 1
sleep 1

echo "  checking that it still runs..." >>$LOG
ps -p $PID >/dev/null || exit 1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient localhost:9901 utah 5 >>$LOG &
sleep 1 # let the client connect 

echo "  killing it..." >>$LOG
kill $PID
sleep 1

echo "  checking that it is dead" >>$LOG
ps -p $PID | grep $PID >/dev/null && exit 1

# clean up
rm -rf reg zebrasrv.pid
