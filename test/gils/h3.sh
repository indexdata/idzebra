# h3.sh - studying the "timing" problem

# timing1.sh - tests that updates are reflected immediately
# in the registers. Repeatedly modifies a record and counts hits.

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
../../index/zebrasrv -S -c h2.cfg -l srv.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1

echo "  update 1..."
../../index/zebraidx -l idx.log -c h2.cfg update records || exit 1

echo "  search 1..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 9$" log || exit 1

echo "making a test record..."
cp records/esdd0006.grs records/esdd0002.grs

echo "  indexing it..."
../../index/zebraidx -l idx.log -c h2.cfg update records || exit 1

echo "  search 2..."
../testclient localhost:9901 "@attr 1=4 utah" > log || exit 1
grep "^Result count: 10$" log || exit 1

echo "  1a: modifying a test record (xyz)..."
cat records/esdd0002.grs | 
  sed 's/UTAH/XYZ/g' |
  sed 's/ESDD0006/ESDD0002/g' |
  sed 's/EARTHQUAKE/TESTRECORD/g'  >/tmp/esdd0002x.grs
cat /tmp/esdd0002x.grs >records/esdd0002.grs

echo "    indexing it..."
../../index/zebraidx -l idx.log -c h2.cfg update records/esdd0002.grs || exit 1
#grep XYZ records/esdd0002.grs  # shows that indeed have XYZ in the data

echo "    search 3..."
../testclient localhost:9901 "@attr 1=4 xyz" > log || exit 1
cat log
grep -q "^Result count: 1$" log || echo "ERROR ---> Result count should have been 1"

echo "stopping server..."
kill -9 `cat zebrasrv.pid` 
sleep 1

echo "  starting a new server..."
../../index/zebrasrv -S -c h2.cfg -l srv2.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1

echo "    search 4..."
../testclient localhost:9901 "@attr 1=4 xyz" > log || exit 1
cat log
grep -q "^Result count: 1$" log || echo "ERROR ---> Result count should have been 1"


echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill -9 `cat zebrasrv.pid` || exit 1
rm -f idx.log log
rm -f records/esdd000[12].grs 
rm -f zebrasrv.pid
rm -f srv.log
rm -f /tmp/esdd0002x.grs

echo ok
