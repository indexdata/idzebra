test -f zebrasrv.pid || exit 1
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 9$" log || exit 1
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 9$" log || exit 1
cp records/esdd0006.grs records/esdd0001.grs
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 10$" log || exit 1
touch records/esdd0001.grs
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 10$" log || exit 1
mv records/esdd0001.grs records/esdd0002.grs
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 10$" log || exit 1
sleep 1
sed 's/UTAH/XYZ/g' <records/esdd0002.grs >records/esdd0002x.grs
mv records/esdd0002x.grs records/esdd0002.grs
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 2
grep "^Result count: 9$" log || exit 1
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 xyz" > log || exit 2
grep "^Result count: 1$" log || exit 1
