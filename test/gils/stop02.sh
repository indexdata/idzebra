#!/bin/sh
# $Id: stop02.sh,v 1.8 2004-12-04 01:38:50 adam Exp $
# test start and stop of the server with -S

pp=${srcdir:-"."}

LOG=stop02.log

rm -f $LOG
echo "initializing" >>$LOG
test -d reg || mkdir reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f z.pid && kill -9 `cat z.pid`

echo "Starting server with -S (static)..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $pp/zebra1.cfg -l $LOG unix:socket
echo "  checking that it runs... " >>$LOG
test -f z.pid || sleep 2 || test -f z.pid || exit 1
PID=`cat z.pid`
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG || exit 1
sleep 1

echo "  checking that it still runs..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting again, with a delay..." >>$LOG
../api/testclient -d 5 unix:socket utah >>$LOG &
sleep 1 # let the client connect 

echo "  killing it..." >>$LOG
kill $PID

sleep 1
echo "  checking that it is dead..." >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && exit 1

# clean up
rm -rf reg z.pid
