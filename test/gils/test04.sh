# test04.sh - test start and stop of the forked server 
#

echo "initializing"
mkdir -p reg
rm -f idx.log srv.log
../../index/zebraidx -l idx.log -c zebra1.cfg init || exit 1

#create a base to test on
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 1

#kill old server (if any)
test -f zebrasrv.pid && kill -9 `cat zebrasrv.pid`
rm -f zebrasrv.pid
rm -f srv.log

echo "Starting server with (forked)..."
../../index/zebrasrv  -c zebra1.cfg -l srv.log tcp:@:9901 &
sleep 1

echo "  cheking that it runs... "
test -f zebrasrv.pid || exit 1
PID=`cat zebrasrv.pid`
ps -p $PID >/dev/null || exit 1

echo "  connecting to it..."
../testclient localhost:9901 utah > log || exit 1
sleep 1

echo "  checking that it still runs..."
ps -p $PID >/dev/null || exit 1

echo "  connecting again, with a delay..."
../testclient localhost:9901 utah 5 > log &
sleep 1 # let the client connect 

echo "  killing it..."
kill  $PID
sleep 1

echo "  checking that the server is dead..."
ps -p $PID >/dev/null && exit 1

echo ok
# clean up
rm -rf reg idx.log srv.log zebrasrv.pid
