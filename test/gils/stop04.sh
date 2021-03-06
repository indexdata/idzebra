#!/bin/sh
# test start and stop of the forked server 

srcdir=${srcdir:-"."}

if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=stop04.log

rm -f $LOG
echo "initializing" >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update records  || exit 1

echo "Starting server with (forked)..." >>$LOG
../../index/zebrasrv -D -p z.pid -c $srcdir/zebra1.cfg -l $LOG unix:socket

echo "  checking that it runs... " >>$LOG
test -f z.pid || exit 1
PID=`cat z.pid`
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG || exit 1
sleep 1

echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient unix:socket utah 5 >>$LOG
sleep 1 # let the client connect 

echo "  killing it..." >>$LOG
kill $PID
sleep 1

echo "  checking that it is dead..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && sleep 1 && \
kill -CHLD $PID >/dev/null 2>&1 && exit 1


# clean up
rm -rf reg z.pid
