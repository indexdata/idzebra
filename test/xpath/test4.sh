#!/bin/sh
# $Id: test4.sh,v 1.3 2004-06-15 09:43:34 adam Exp $

pp=${srcdir:-"."}

LOG=test4.log
TMP1=test4-1.tmp
TMP2=test4-2.tmp
TMP3=test4-3.tmp
TMP4=test4-4.tmp
TMP5=test4-5.tmp
rm -f $LOG
rm -f $TMP
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra.cfg -l $LOG -t grs.sgml update $pp/rec4.xml || exit 2
test -f dict*.mf || exit 1
../../index/zebrasrv -c $pp/zebra.cfg -l $LOG -S unix:socket & 
sleep 1
test -f zebrasrv.pid || exit 2
../api/testclient unix:socket '@attr 1=/root content' >$TMP1
../api/testclient unix:socket '@attr 1=/root/first content' >$TMP2
../api/testclient unix:socket "@attr {1=/root/first[@attr='danish']} content" >$TMP3
../api/testclient unix:socket "@attr {1=/root/second[@attr='danish lake']} content" >$TMP4
../api/testclient unix:socket "@attr {1=/root/third[@attr='dansk sø']} content" >$TMP5
echo 'Killing server' >>$LOG
kill `cat zebrasrv.pid` || exit 3
cat $TMP1 $TMP2 $TMP3 $TMP4 $TMP5 >>$LOG
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP1 >/dev/null || exit 4
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP2 >/dev/null || exit 5
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP3 >/dev/null || exit 6
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP4 >/dev/null || exit 7
echo 'Checking that result count is 1' >>$LOG
grep "^Result count: 1$" $TMP5 >/dev/null || exit 8
echo 'Test OK' >>$LOG
exit 0

