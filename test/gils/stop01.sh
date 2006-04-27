#!/bin/sh
# $Id: stop01.sh,v 1.12 2006-04-27 10:52:26 marc Exp $
# test start and stop of the server with -1

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
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update records  || exit 1

echo "Starting server with -1 (one shot)..." >>$LOG
../../index/zebrasrv -D -p z.pid -1 -c $srcdir/zebra1.cfg -l $LOG unix:socket
echo "  checking that it runs... " >>$LOG
test -f z.pid || exit 1
PID=`cat z.pid`
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG || exit 1

echo "  checking that server does not run any more" >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && sleep 1 && \
kill -CHLD $PID >/dev/null 2>&1 && exit 1

# clean up
rm -rf reg z.pid
