#!/bin/sh
# test start and stop of the server with unix socket and one shot -1

set -e
srcdir=${srcdir:-"."}
if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=stop01.log
rm -f $LOG
echo "initializing" >>$LOG
test -d reg || mkdir reg
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init

#create a base to test on
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update records

echo "Starting server with -1 (one shot)..." >>$LOG
../../index/zebrasrv -D -p z.pid -1 -c $srcdir/zebra1.cfg -l $LOG unix:socket
echo "  checking that it runs... " >>$LOG
test -f z.pid
. "$srcdir/cleanup.sh"
kill -CHLD $PID >/dev/null 2>&1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG

echo "  checking that server does not run any more" >>$LOG
if kill -CHLD "$PID" >/dev/null 2>&1; then
    sleep 1
    if kill -CHLD "$PID" >/dev/null 2>&1; then
        exit 1
    fi
fi

# clean up
rm -rf reg
