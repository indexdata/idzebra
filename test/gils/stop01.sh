#!/bin/sh
# $Id: stop01.sh,v 1.9 2004-12-04 01:48:26 adam Exp $
# test start and stop of the server with -1

pp=${srcdir:-"."}

LOG=stop01.log

rm -f $LOG
echo "initializing" >>$LOG
test -d reg || mkdir reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f z.pid && kill -9 `cat z.pid`

echo "Starting server with -1 (one shot)..." >>$LOG
../../index/zebrasrv -D -p z.pid -1 -c $pp/zebra1.cfg -l $LOG unix:socket
echo "  checking that it runs... " >>$LOG
test -f z.pid || sleep 1 || test -f z.pid || exit 1
PID=`cat z.pid`
kill -CHLD $PID >/dev/null 2>&1 || exit 1

echo "  connecting to it..." >>$LOG
../api/testclient unix:socket utah >>$LOG || exit 1

echo "  checking that server does not run any more" >>$LOG
kill -CHLD $PID >/dev/null 2>&1 && sleep 1 && \
kill -CHLD $PID >/dev/null 2>&1 && exit 1

# clean up
rm -rf reg z.pid
