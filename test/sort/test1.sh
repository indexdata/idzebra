#!/bin/sh
# $Id: test1.sh,v 1.8 2004-07-28 09:47:41 adam Exp $

pp=${srcdir:-"."}

ulimit -c 10000
LOG=test1.log
rm -fr lock
mkdir lock
rm -fr reg
mkdir reg
rm -fr recs
mkdir recs
cp $pp/rec*.xml recs
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update recs || exit 1
../../index/zebrasrv -c $pp/zebra.cfg -l $LOG unix:socket &
sleep 1
test -f lock/zebrasrv.pid || exit 2
../api/testclient -n4 unix:socket '@or computer @attr 7=1 @attr 1=30 0' >tmp1
echo 'Result count: 4
my:
  title: 3rd computer
my:
  title: third ^computer
my:
  title: second computer
  dateTime: 1
my:
  title: first computer
  dateTime: 2' >tmp2

kill `cat lock/zebrasrv.pid`
diff tmp1 tmp2
