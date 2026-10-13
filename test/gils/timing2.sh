#!/bin/sh
# Demonstrates that updates depend on file timestamps.

set -e

srcdir=${srcdir:-"."}
if [ "$srcdir" != "." ]
    then
    echo "Jumping over test"
    exit 0
fi

LOG=timing2.log
rm -f $LOG

echo "  init..." >>$LOG
rm -f timeref[12]
# these should not be here, will be created later
$srcdir/cleanrecords.sh

../../index/zebraidx -c $srcdir/zebra2.cfg -l $LOG init
touch timeref1  # make an early timestamp

echo "  starting server..." >>$LOG
../../index/zebrasrv -D -p z.pid -S -c $srcdir/zebra2.cfg -l $LOG unix:socket
test -f z.pid
. "$srcdir/cleanup.sh"

# Zebra stores file mtimes in whole seconds; separate the reference timestamps.
sleep 1
touch timeref2  # make a later timestamp

echo "  update 1..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "  search 1..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >>$LOG

echo "making a test record..." >>$LOG
cp $srcdir/records/esdd0006.grs $srcdir/records/esdd0002.grs
touch -r timeref1 $srcdir/records/esdd0002.grs

echo "  indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "  search 2..." >>$LOG
../api/testclient -c 10 unix:socket "@attr 1=4 utah" >>$LOG

echo "  modifying a test record (xyz)..." >>$LOG
sed 's/UTAH/XYZ/g' <$srcdir/records/esdd0002.grs >$srcdir/records/esdd0002x.grs
mv $srcdir/records/esdd0002x.grs $srcdir/records/esdd0002.grs
touch -r timeref1 $srcdir/records/esdd0002.grs # reset timestamp to 'early'

echo "    not indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "    search 3..." >>$LOG
../api/testclient -c 10 unix:socket "@attr 1=4 utah" >>$LOG

echo "  touching its timestamp..." >>$LOG
touch -r timeref2 $srcdir/records/esdd0002.grs # set timestamp to 'late'

echo "    indexing it..." >>$LOG
../../index/zebraidx -l $LOG -c $srcdir/zebra2.cfg update $srcdir/records

echo "    search 4..." >>$LOG
../api/testclient -c 9 unix:socket "@attr 1=4 utah" >>$LOG

rm -f timeref[12]
