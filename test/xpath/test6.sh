#!/bin/sh
# $Id: test6.sh,v 1.1.2.3 2006-02-06 23:11:12 adam Exp $

pp=${srcdir:-"."}

LOG="test6.log"
TMP="test6.tmp"
DBG=""

rm -f $LOG
rm -f $TMP.*
../../index/zebraidx -c $pp/zebra6.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra6.cfg -l $LOG -t grs.xml update $pp/rec61.xml $pp/rec62.xml || exit 2
test -f dict*.mf || exit 1

../../index/zebrasrv -c $pp/zebra6.cfg -l $LOG $DBG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2

../api/testclient unix:socket \
            '@attr 5=1 @attr 6=3  @attr 4=1 @attr 1=/assembled/basic/names/CASno "367-93-1"' >$TMP.1

../api/testclient unix:socket \
            '@attr 5=1 @attr 6=3  @attr 4=1 @attr 1=18 "367-93-1"' >$TMP.2

../api/testclient unix:socket '@attr 1=/assembled/orgs/org 0' >$TMP.3

../api/testclient unix:socket '@and @attr 1=/assembled/orgs/org 0 @attr 5=1 @attr 6=3 @attr 4=1 @attr 1=/assembled/basic/names/CASno "367-93-1"' >$TMP.4

../api/testclient unix:socket '@and @attr 1=/assembled/orgs/org 46 @attr 5=1 @attr 6=3  @attr 4=1 @attr 1=/assembled/basic/names/CASno 367-93-1' >$TMP.5

# bug 431
../api/testclient unix:socket '@attr 1=1021 46' >$TMP.6

echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
sleep 1
echo "Result counts: " >> $LOG
cat $TMP.* >>$LOG
echo 'Checking that result 1 count is 2' >>$LOG
grep "^Result count: 2$" $TMP.1 >/dev/null || exit 4
echo 'Checking that result 2 count is 2' >>$LOG
grep "^Result count: 2$" $TMP.2 >/dev/null || exit 5
echo 'Checking that result 3 count is 1' >>$LOG
grep "^Result count: 1$" $TMP.3 >/dev/null || exit 6
echo 'Checking that result 4 count is 1' >>$LOG
grep "^Result count: 1$" $TMP.4 >/dev/null || exit 7
echo 'Checking that result 5 count is 2' >>$LOG
grep "^Result count: 2$" $TMP.5 >/dev/null || exit 8
echo 'Checking that result 6 count is 1' >>$LOG
grep "^Result count: 1$" $TMP.6 >/dev/null || exit 9
echo 'Test OK' >>$LOG
exit 0

