#!/bin/sh
# $Id: test1.sh,v 1.1.2.1 2004-09-16 14:07:51 adam Exp $

pp=${srcdir:-"."}

ulimit -c 10000
LOG=test1.log
rm -f $LOG
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
../api/testclient -n4 unix:socket '@or computer @attr 7=1 @attr 1=4 0' >tmp1

kill `cat lock/zebrasrv.pid`

echo 'Result count: 4
my:
  title: first computer
my:
  title: the fourth computer
my:
  title: second computer
my:
  title: A third computer' >tmp2

diff tmp1 tmp2
