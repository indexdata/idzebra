#!/bin/sh
# $Id: simple1.sh,v 1.4 2004-06-15 09:43:34 adam Exp $
# test with one simple sgml record

pp=${srcdir:-"."}

LOG=simple1.log
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/simple1.xml || exit 1

echo "Starting server " >> $LOG
../../index/zebrasrv -c $pp/zebra.cfg -l $LOG -S unix:socket &
sleep 1
test -f zebrasrv.pid || exit 2

../api/testclient unix:socket -c 0 '@attr 1=/sgml/tag before' >> $LOG || exit 1
../api/testclient unix:socket -c 1 '@attr 1=/sgml/tag inside' >> $LOG || exit 1
../api/testclient unix:socket -c 0 '@attr 1=/sgml/tag after'  >> $LOG || exit 1

../api/testclient unix:socket -c 0 '@attr 1=/sgml/none after'  >> $LOG || exit 1

../api/testclient unix:socket -c 1 '@attr 1=/sgml before' >> $LOG || exit 1
../api/testclient unix:socket -c 1 '@attr 1=/sgml inside' >> $LOG || exit 1
../api/testclient unix:socket -c 1 '@attr 1=/sgml after'  >> $LOG || exit 1

echo "killing server " >> $LOG
kill `cat zebrasrv.pid` || exit 3
echo ok >> $LOG
