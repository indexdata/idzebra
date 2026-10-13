#!/bin/sh
# test start and stop of the forked server

set -e
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
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init

#create a base to test on
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update records

echo "Starting server with (forked)..." >>$LOG
../../index/zebrasrv -D -p z.pid -c $srcdir/zebra1.cfg -l $LOG unix:socket

echo "  checking that it runs... " >>$LOG
test -f z.pid || exit 1
. "$srcdir/cleanup.sh"
kill -CHLD $PID >/dev/null 2>&1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG
sleep 1

echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient -d 5 unix:socket utah >>$LOG &
CLIENT_PID=$!
sleep 1 # let the client connect

echo "  killing it..." >>$LOG
kill $PID
sleep 1

echo "  checking that it is dead..." >>$LOG
if kill -CHLD $PID >/dev/null 2>&1; then
    sleep 1
    if kill -CHLD $PID >/dev/null 2>&1; then
        exit 1
    fi
fi

# clean up
rm -rf reg
