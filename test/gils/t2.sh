echo "killing old server (if any)..."
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log
echo "starting server..."
../../index/zebrasrv -S -c zebra1.cfg -l srv.log tcp:@:9901 &
sleep 1
echo "cheking it runs..."
test -f zebrasrv.pid || exit 1
echo "ok"
