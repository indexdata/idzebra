echo "checking server is running..."
test -f zebrasrv.pid || exit 1

echo "search 1..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 9$" log || exit 1

echo "indexing records..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 3

echo "search 2..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 4
grep "^Result count: 9$" log || exit 1

echo "making test records..."
cp records/esdd0006.grs records/esdd0001.grs

echo "indexing them..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 5

echo "search 3..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 6
grep "^Result count: 10$" log || exit 1
touch records/esdd0001.grs

echo "indexing again..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 7

echo "search 4..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 8
grep "^Result count: 10$" log || exit 1

echo "making another test record..."
mv records/esdd0001.grs records/esdd0002.grs

echo "indexing it too..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 9

echo "search 5..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 10
grep "^Result count: 10$" log || exit 1

echo "modifying a test record..."
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs

echo "indexing it..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 11

echo "search 6..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 12
grep "^Result count: 9$" log || exit 1

echo "search 7..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 xyz" > log || exit 13
grep "^Result count: 1$" log || exit 1

echo ok
