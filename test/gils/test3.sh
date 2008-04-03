#!/bin/sh

# Testing searches with lots of @and operators
# in order to test the fast-forward operation of rsets

srcdir=${srcdir:-"."}

if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=test3.log
DBG="-v 1647"

rm -f $LOG

# these should not be here, will be created later
$srcdir/cleanrecords.sh

echo  "initializing..." >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init || exit 1

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records  || exit 1

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra1.cfg $DBG -l $LOG unix:socket

echo "checking it runs..." >>$LOG
test -f z.pid || exit 1

echo "search A1..." >>$LOG
../api/testclient -c 17 unix:socket utah > log || exit 1

echo "search A2..." >>$LOG
../api/testclient -c 31 unix:socket the > log || exit 1

echo "search A3..." >>$LOG
../api/testclient -c 4 unix:socket deposits > log || exit 1


echo "search B1..." >>$LOG
../api/testclient -c 7 unix:socket "@and utah the" > log || exit 1

echo "search B2..." >>$LOG
../api/testclient -c 7 unix:socket "@and the utah" > log || exit 1

echo "search C1..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and the utah epicenter" > log || exit 1

echo "search C2..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and the epicenter utah" > log || exit 1

echo "search C3..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and utah the epicenter" > log || exit 1

echo "search C4..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and utah epicenter the" > log || exit 1

echo "search C5..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and epicenter the utah" > log || exit 1

echo "search C6..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and epicenter utah the" > log || exit 1


echo "search D1..." >>$LOG
../api/testclient -c 30 unix:socket "@and the of "  > log || exit 1

echo "search D2..." >>$LOG
../api/testclient -c 30 unix:socket "@and of the"  > log || exit 1

echo "search D3..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and the of of"  > log || exit 1

echo "search D4..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and of the the"  > log || exit 1

echo "search D5..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and @and the of of the"  > log || exit 1

echo "search D6..." >>$LOG
../api/testclient -c 15 unix:socket '@and @and in the data' > log || exit 1

echo "search D7..." >>$LOG
../api/testclient -c 15 unix:socket '@and @and in data the' > log || exit 1

echo "search D8..." >>$LOG
../api/testclient -c 2 unix:socket "@attr 1=4 @and UNPUBLISHED @and REPORTS AND" >log || exit 1
# This one failed at early fast-forwards

echo "search E1..." >>$LOG
../api/testclient -c 41 unix:socket "@or the utah"  > log || exit 1

echo "search E2..." >>$LOG
../api/testclient -c 41 unix:socket "@or utah the"  > log || exit 1

echo "search E3..." >>$LOG
../api/testclient -c 42 unix:socket "@or deposits @or the utah"  > log || exit 1

echo "search E4..." >>$LOG
../api/testclient -c 3 unix:socket "@and deposits @or the utah"  > log || exit 1

echo "search E5..." >>$LOG
../api/testclient -c 3 unix:socket "@and @or the utah deposits"  > log || exit 1

echo "search F1..." >>$LOG
../api/testclient -c 24 unix:socket "@not the utah "  > log || exit 1

echo "search F2..." >>$LOG
../api/testclient -c 10 unix:socket "@not utah the "  > log || exit 1

echo "search F3..." >>$LOG
../api/testclient -c 1 unix:socket "@and deposits @not utah the "  > log || exit 1

echo "search F4..." >>$LOG
../api/testclient -c 1 unix:socket "@and @not utah the deposits"  > log || exit 1


echo "stopping server..." >>$LOG
test -f z.pid || exit 1
kill `cat z.pid` || exit 1
rm -f z.pid
sleep 1

echo "Test successfully completed" >>$LOG

