#!/bin/sh
# $Id: test3.sh,v 1.5 2005-01-02 23:21:31 adam Exp $

# Testing searches with lots of @and operators
# in order to test the fast-forward operation of rsets

pp=${srcdir:-"."}

LOG=test3.log
DBG="-v 1647"

rm -f $LOG

echo  "initializing..." >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg init || exit 1

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $pp/zebra1.cfg update $pp/records  || exit 1

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $pp/zebra1.cfg $DBG -l $LOG tcp:@:9901
sleep 1

echo "checking it runs..." >>$LOG
test -f z.pid || exit 1

echo "search A1..." >>$LOG
../api/testclient -c 17 localhost:9901 utah > log || exit 1

echo "search A2..." >>$LOG
../api/testclient -c 30 localhost:9901 the > log || exit 1

echo "search A3..." >>$LOG
../api/testclient -c 4 localhost:9901 deposits > log || exit 1


echo "search B1..." >>$LOG
../api/testclient -c 7 localhost:9901 "@and utah the" > log || exit 1

echo "search B2..." >>$LOG
../api/testclient -c 7 localhost:9901 "@and the utah" > log || exit 1

echo "search C1..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and the utah epicenter" > log || exit 1

echo "search C2..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and the epicenter utah" > log || exit 1

echo "search C3..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and utah the epicenter" > log || exit 1

echo "search C4..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and utah epicenter the" > log || exit 1

echo "search C5..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and epicenter the utah" > log || exit 1

echo "search C6..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @and epicenter utah the" > log || exit 1


echo "search D1..." >>$LOG
../api/testclient -c 29 localhost:9901 "@and the of "  > log || exit 1

echo "search D2..." >>$LOG
../api/testclient -c 29 localhost:9901 "@and of the"  > log || exit 1

echo "search D3..." >>$LOG
../api/testclient -c 29 localhost:9901 "@and @and the of of"  > log || exit 1

echo "search D4..." >>$LOG
../api/testclient -c 29 localhost:9901 "@and @and of the the"  > log || exit 1

echo "search D5..." >>$LOG
../api/testclient -c 29 localhost:9901 "@and @and @and the of of the"  > log || exit 1

echo "search D6..." >>$LOG
../api/testclient -c 15 localhost:9901 '@and @and in the data' > log || exit 1

echo "search D7..." >>$LOG
../api/testclient -c 15 localhost:9901 '@and @and in data the' > log || exit 1

echo "search D8..." >>$LOG
../api/testclient -c 2 localhost:9901 "@attr 1=4 @and UNPUBLISHED @and REPORTS AND" >log || exit 1
# This one failed at early fast-forwards

echo "search E1..." >>$LOG
../api/testclient -c 40 localhost:9901 "@or the utah"  > log || exit 1

echo "search E2..." >>$LOG
../api/testclient -c 40 localhost:9901 "@or utah the"  > log || exit 1

echo "search E3..." >>$LOG
../api/testclient -c 42 localhost:9901 "@or deposits @or the utah"  > log || exit 1

echo "search E4..." >>$LOG
../api/testclient -c 2 localhost:9901 "@and deposits @or the utah"  > log || exit 1

echo "search E5..." >>$LOG
../api/testclient -c 2 localhost:9901 "@and @or the utah deposits"  > log || exit 1

echo "search F1..." >>$LOG
../api/testclient -c 23 localhost:9901 "@not the utah "  > log || exit 1

echo "search F2..." >>$LOG
../api/testclient -c 10 localhost:9901 "@not utah the "  > log || exit 1

echo "search F3..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and deposits @not utah the "  > log || exit 1

echo "search F4..." >>$LOG
../api/testclient -c 1 localhost:9901 "@and @not utah the deposits"  > log || exit 1


echo "stopping server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f z.pid
sleep 1

echo "Test successfully completed" >>$LOG

