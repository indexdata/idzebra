# Source after the server has started and written z.pid.
# LOG and srcdir are supplied by the test; CLIENT_PID is optional.

PID=$(cat z.pid)

cleanup()
{
    status=$?
    trap - 0
    trap '' INT TERM
    set +e
    echo "stopping server..." >>"$LOG"
    kill "$PID" 2>/dev/null
    attempts=0
    stopped=1
    while kill -0 "$PID" 2>/dev/null; do
        if [ "$attempts" -ge 5 ]; then
            echo "server $PID did not stop" >>"$LOG"
            stopped=0
            if [ "$status" -eq 0 ]; then
                status=1
            fi
            break
        fi
        sleep 1
        attempts=$((attempts + 1))
        # A signal during request handling may need repeating to wake the loop.
        kill "$PID" 2>/dev/null
    done
    if [ -n "${CLIENT_PID:-}" ]; then
        kill "$CLIENT_PID" 2>/dev/null
        wait "$CLIENT_PID" 2>/dev/null
    fi
    if [ "$stopped" -eq 1 ]; then
        rm -f z.pid
    fi
    if ! "$srcdir/cleanrecords.sh"; then
        if [ "$status" -eq 0 ]; then
            status=1
        fi
    fi
    exit "$status"
}
trap cleanup 0
trap 'exit 130' INT
trap 'exit 143' TERM
