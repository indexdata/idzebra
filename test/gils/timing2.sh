# timing2.sh 
# 2: Test without sleeps, fails around step 1a or 1b.

echo "Testing timings of updates"
echo "  init..."
rm -f idx.log log
rm -f records/esdd000[12].grs # these should not be here, will be created later
../../index/zebraidx -l idx.log init || exit 1

echo "  killing old server (if any)..."
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "  starting server..."
../../index/zebrasrv -S -c zebra2.cfg -l srv.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1

echo "  update 1..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "  search 1..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "making a test record..."
cp records/esdd0006.grs records/esdd0002.grs

echo "  indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "  search 2..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1
echo "  1a: modifying a test record (xyz)..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 3..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 9$" log || exit 1

echo "  1b: modifying the test record back (utah)..."
sed 's/XYZ/UTAH/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 4..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 10$" log || exit 1

echo "  2a: modifying the test record (xyz)..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 5..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 9$" log || exit 1

echo "  2b: modifying the test record back (utah)..."
sed 's/XYZ/UTAH/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 6..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 10$" log || exit 1


echo "  3a: modifying the test record (xyz)..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 7..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 9$" log || exit 1

echo "  3b: modifying the test record back (utah)..."
sed 's/XYZ/UTAH/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 1

echo "    search 8..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
echo "    checking..."
grep "^Result count: 10$" log || exit 1


echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill -9 `cat zebrasrv.pid` || exit 1
rm -f idx.log log
rm -f records/esdd000[12].grs 
rm -f zebrasrv.pid
rm -f srv.log

echo ok
