echo "stopping server..."
test -f zebrasrv.pid || exit 1
kill -9 `cat zebrasrv.pid` || exit 2

echo "ok"
