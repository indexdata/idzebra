#!/bin/sh
# $Id: simple1.sh,v 1.2 2003-05-06 17:39:01 adam Exp $
# test with one simple sgml record

LOG=simple1.log
../../index/zebraidx -l $LOG init || exit 1
../../index/zebraidx -l $LOG update simple1.rec || exit 1

echo "Starting server " >> $LOG
../../index/zebrasrv -l $LOG -S unix:socket &
sleep 1
test -f zebrasrv.pid || exit 2

../testclient unix:socket -c 0 '@attr 1=/sgml/tag before' >> $LOG || exit 1
../testclient unix:socket -c 1 '@attr 1=/sgml/tag inside' >> $LOG || exit 1
../testclient unix:socket -c 0 '@attr 1=/sgml/tag after'  >> $LOG || exit 1

../testclient unix:socket -c 0 '@attr 1=/sgml/none after'  >> $LOG || exit 1

../testclient unix:socket -c 1 '@attr 1=/sgml before' >> $LOG || exit 1
../testclient unix:socket -c 1 '@attr 1=/sgml inside' >> $LOG || exit 1
../testclient unix:socket -c 1 '@attr 1=/sgml after'  >> $LOG || exit 1

echo "killing server " >> $LOG
kill `cat zebrasrv.pid` || exit 3
echo ok >> $LOG
