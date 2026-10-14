#!/bin/sh

# Testing searches with lots of @and operators
# in order to test the fast-forward operation of rsets

set -e
srcdir=${srcdir:-"."}
if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

DBG="-v 1647"
LOG=test3.log
rm -f $LOG

# these should not be here, will be created later
$srcdir/cleanrecords.sh

echo  "initializing..." >>$LOG
mkdir -p reg
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg init

echo "updating..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra1.cfg update $srcdir/records

echo "starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra1.cfg $DBG -l $LOG unix:socket

echo "checking it runs..." >>$LOG
test -f z.pid
. "$srcdir/cleanup.sh"

echo "search A1..." >>$LOG
../api/testclient -c 17 unix:socket utah >>$LOG

echo "search A2..." >>$LOG
../api/testclient -c 31 unix:socket the >>$LOG

echo "search A3..." >>$LOG
../api/testclient -c 4 unix:socket deposits >>$LOG

echo "search B1..." >>$LOG
../api/testclient -c 7 unix:socket "@and utah the" >>$LOG

echo "search B2..." >>$LOG
../api/testclient -c 7 unix:socket "@and the utah" >>$LOG

echo "search C1..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and the utah epicenter" >>$LOG

echo "search C2..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and the epicenter utah" >>$LOG

echo "search C3..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and utah the epicenter" >>$LOG

echo "search C4..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and utah epicenter the" >>$LOG

echo "search C5..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and epicenter the utah" >>$LOG

echo "search C6..." >>$LOG
../api/testclient -c 1 unix:socket "@and @and epicenter utah the" >>$LOG

echo "search D1..." >>$LOG
../api/testclient -c 30 unix:socket "@and the of " >>$LOG

echo "search D2..." >>$LOG
../api/testclient -c 30 unix:socket "@and of the" >>$LOG

echo "search D3..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and the of of" >>$LOG

echo "search D4..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and of the the" >>$LOG

echo "search D5..." >>$LOG
../api/testclient -c 30 unix:socket "@and @and @and the of of the" >>$LOG

echo "search D6..." >>$LOG
../api/testclient -c 15 unix:socket '@and @and in the data' >>$LOG

echo "search D7..." >>$LOG
../api/testclient -c 15 unix:socket '@and @and in data the' >>$LOG

echo "search D8..." >>$LOG
../api/testclient -c 2 unix:socket "@attr 1=4 @and UNPUBLISHED @and REPORTS AND" >log
# This one failed at early fast-forwards

echo "search E1..." >>$LOG
../api/testclient -c 41 unix:socket "@or the utah" >>$LOG

echo "search E2..." >>$LOG
../api/testclient -c 41 unix:socket "@or utah the" >>$LOG

echo "search E3..." >>$LOG
../api/testclient -c 42 unix:socket "@or deposits @or the utah" >>$LOG

echo "search E4..." >>$LOG
../api/testclient -c 3 unix:socket "@and deposits @or the utah" >>$LOG

echo "search E5..." >>$LOG
../api/testclient -c 3 unix:socket "@and @or the utah deposits" >>$LOG

echo "search F1..." >>$LOG
../api/testclient -c 24 unix:socket "@not the utah " >>$LOG

echo "search F2..." >>$LOG
../api/testclient -c 10 unix:socket "@not utah the " >>$LOG

echo "search F3..." >>$LOG
../api/testclient -c 1 unix:socket "@and deposits @not utah the " >>$LOG

echo "search F4..." >>$LOG
../api/testclient -c 1 unix:socket "@and @not utah the deposits" >>$LOG
