#!/bin/sh
# $Id: stop01.sh,v 1.7 2004-09-24 15:03:19 adam Exp $
# test start and stop of the server with -1

pp=${srcdir:-"."}

LOG=stop01.log

rm -f $LOG
echo "initializing" >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`

echo "Starting server with -1 (one shot)..." >>$LOG
../../index/zebrasrv -1 -c $pp/zebra1.cfg -l $LOG tcp:@:9901 &
sleep 1
echo "  checking that it runs... " >>$LOG
test -f zebrasrv.pid || sleep 5 || test -f zebrasrv.pid || exit 1
PID=`cat zebrasrv.pid`
ps -p $PID |grep $PID >/dev/null || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient localhost:9901 utah >>$LOG || exit 1
sleep 1

echo "  checking that server does not run any more" >>$LOG
ps -p $PID | grep $PID >/dev/null && exit 1

# clean up
rm -rf reg zebrasrv.pid
