test -f zebrasrv.pid || exit 1
kill `cat zebrasrv.pid` || exit 2
