#!/bin/sh
# $Id: stop03.sh,v 1.10 2005-01-03 09:19:26 adam Exp $
# test start and stop of the threaded server (-T)

pp=${srcdir:-"."}

LOG=stop03.log

rm -f $LOG
echo "initializing" >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update records  || exit 1

echo "Starting server with -T (threaded)..." >>$LOG
../../index/zebrasrv -D -p z.pid -T -c $pp/zebra1.cfg -l $LOG tcp:@:9901 2>out

if grep 'not available' out >/dev/null; then
    test -f z.pid && rm -f z.pid
    exit 0
fi

echo "  checking that it runs..." >>$LOG
test -f z.pid || exit 1

PID=`cat z.pid`
echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient localhost:9901 utah >>$LOG || exit 1
sleep 1

echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient localhost:9901 utah 5 >>$LOG &
sleep 1 # let the client connect 

echo "  killing it..." >>$LOG
kill $PID
sleep 1

echo "  checking that it is dead..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && sleep 1 && \
kill -CHLD $PID >/dev/null 2>&1 && exit 1

# clean up
rm -rf reg z.pid
