echo "Checking taht server runs..."
test -f zebrasrv.pid || exit 1

echo "search 1..."
../../../yaz/zoom/zoomtst1 localhost:9901 utah > log || exit 3
grep "^Result count: 17$" log || exit 4

echo "search 2..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@or utah the" > log || exit 5
grep "^Result count: 40$" log || exit 6

echo "search 3..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 the" > log || exit 7
grep "^Result count: 1$" log || exit 8

echo "search 4..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 9
grep "^Result count: 9$" log || exit 10

echo "reindexing..."
../../index/zebraidx -l idx.log -c  zebra1.cfg update records || exit 11

echo "search 5..."
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 12
grep "^Result count: 18$" log || exit 14

echo "ok"
