#!/bin/sh
# $Id: stop04.sh,v 1.9 2005-01-03 09:19:26 adam Exp $
# test start and stop of the forked server 

pp=${srcdir:-"."}

LOG=stop04.log

rm -f $LOG
echo "initializing" >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update records  || exit 1

echo "Starting server with (forked)..." >>$LOG
../../index/zebrasrv -D -p z.pid -c $pp/zebra1.cfg -l $LOG tcp:@:9901 

echo "  checking that it runs... " >>$LOG
test -f z.pid || exit 1
PID=`cat z.pid`
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient localhost:9901 utah >>$LOG || exit 1
sleep 1

echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient localhost:9901 utah 5 >>$LOG
sleep 1 # let the client connect 

echo "  killing it..." >>$LOG
kill $PID
sleep 1

echo "  checking that it is dead..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && sleep 1 && \
kill -CHLD $PID >/dev/null 2>&1 && exit 1


# clean up
rm -rf reg z.pid
