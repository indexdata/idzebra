test -f zebrasrv.pid || exit 1
../../../yaz/zoom/zoomtst1 localhost:9901 utah > log || exit 3
grep "^Result count: 17$" log || exit 4
../../../yaz/zoom/zoomtst1 localhost:9901 "@or utah the" > log || exit 5
grep "^Result count: 40$" log || exit 6
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 the" > log || exit 7
grep "^Result count: 1$" log || exit 8
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 9
grep "^Result count: 9$" log || exit 10
../../index/zebraidx -l idx.log -c  zebra1.cfg update records || exit 11
../../../yaz/zoom/zoomtst1 localhost:9901 "@attr 1=4 utah" > log || exit 12
grep "^Result count: 18$" log || exit 14
