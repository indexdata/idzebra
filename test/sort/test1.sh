#!/bin/sh
# $Id: test1.sh,v 1.3 2003-05-21 14:39:22 adam Exp $
LOG=test1.log
rm -fr lock
mkdir lock
rm -fr reg
mkdir reg
rm -fr recs
mkdir recs
cp rec*.xml recs
../../index/zebraidx -l $LOG update recs || exit 1
../../index/zebrasrv -l $LOG unix:socket &
sleep 1
test -f zebrasrv.pid || exit 2
../api/testclient -n3 unix:socket '@or computer @attr 7=1 @attr 1=30 0' >tmp1
echo 'Result count: 3
my:
  title: third computer
my:
  title: second computer
  dateTime: 1
my:
  title: first computer
  dateTime: 2' >tmp2

kill `cat zebrasrv.pid`
diff tmp1 tmp2
