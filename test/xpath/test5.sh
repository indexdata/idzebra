#!/bin/sh
# $Id: test5.sh,v 1.3.2.1 2004-12-17 11:25:33 heikki Exp $

pp=${srcdir:-"."}

LOG="test5.log"
TMP="test5.tmp"
# DBG="-v 1647"
DBG=""

rm -f $LOG
rm -f $TMP.*
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra.cfg -l $LOG -t grs.sgml update $pp/rec5.xml || exit 2
test -f dict*.mf || exit 1
../../index/zebrasrv -c $pp/zebra.cfg -l $LOG $DBG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2
../api/testclient unix:socket '@attr 1=/record/title foo' >$TMP.1
../api/testclient unix:socket '@attr 1=/record/title bar' >$TMP.2
../api/testclient unix:socket "@attr {1=/record/title[@lang='da']} foo" >$TMP.3
../api/testclient unix:socket "@attr {1=/record/title[@lang='en']} foo" >$TMP.4
../api/testclient unix:socket "@attr 1=/record/title @and foo bar" >$TMP.5
# The last one returns two hits, as the and applies to the whole
# record, so it matches <title>foo</title><title>bar</title>
# This might not have to be like that, but currently that is what
# zebra does.
../api/testclient unix:socket "@attr 1=/record/value bar" >$TMP.6
../api/testclient unix:socket "@and @attr 1=/record/title foo @attr 1=/record/title grunt" >$TMP.7

../api/testclient unix:socket "@attr {1=/record/title[@lang='en']} English" >$TMP.8
../api/testclient unix:socket "@attr {1=/record/title[@lang='en']} Danish" >$TMP.9
../api/testclient unix:socket "@attr {1=/record/title[@lang='da']} English" >$TMP.10
../api/testclient unix:socket "@attr {1=/record/title[@lang='da']} Danish" >$TMP.11

echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
sleep 1
echo "Result counts: " >> $LOG
cat $TMP.* >>$LOG
echo 'Checking that result count is 4' >>$LOG
grep "^Result count: 4$" $TMP.1 >/dev/null || exit 4
echo 'Checking that result count is 2' >>$LOG
grep "^Result count: 2$" $TMP.2 >/dev/null || exit 5
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.3 >/dev/null || exit 6
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.4 >/dev/null || exit 7
echo 'Checking that result count is 2' >>$LOG
grep "^Result count: 2$" $TMP.5 >/dev/null || exit 8
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.6 >/dev/null || exit 9
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.7 >/dev/null || exit 10
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.8 >/dev/null || exit 11
echo 'Checking that result count is 0' >>$LOG
grep "^Result count: 0$" $TMP.9 >/dev/null || exit 12
echo 'Checking that result count is 0' >>$LOG
grep "^Result count: 0$" $TMP.10 >/dev/null || exit 13
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP.11 >/dev/null || exit 14
echo 'Test OK' >>$LOG
exit 0

