test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log
../../index/zebrasrv -S -c zebra1.cfg -l srv.log tcp:@:9901 &
sleep 1
test -f zebrasrv.pid || exit 1
