#!/bin/bash

cd "$(dirname "$0")/.." || exit 1

echo "===== Watchdog Tests ====="
FAILED=0

make clean
make || exit 1

echo "===== Test 1: Unexpected Termination ====="

./watchdog > test_crash.txt 2>&1 &
WATCHDOG_PID=$!

sleep 10

kill -TERM "$WATCHDOG_PID"
wait "$WATCHDOG_PID"

if grep -q "exited with code 1" test_crash.txt &&
   grep -q "restarting child (mode crash)" test_crash.txt; then
    echo "PASS: Unexpected termination"
else
    FAILED=1
    echo "FAIL: Unexpected termination"
fi

echo "===== Test 2: Restart Limit ====="

RESTART_COUNT=$(grep -c "restarting child (mode crash)" test_crash.txt)

if [ "$RESTART_COUNT" -eq 3 ] &&
   grep -q "reached max restarts, giving up" test_crash.txt; then
    echo "PASS: Restart limit"
else
    FAILED=1
    echo "FAIL: Restart limit"
fi

echo "===== Test 3: Normal Termination ====="

sed 's/{"crash", "loop"}/{"normal", "loop"}/' \
    src/process_manager.c > tests/process_manager_test.c

gcc -Wall -Wextra -std=c11 \
    -D_POSIX_C_SOURCE=200809L -Iinclude \
    src/main.c src/signal_manager.c \
    tests/process_manager_test.c src/logger.c \
    -o watchdog_test || exit 1

./watchdog_test > test_normal.txt 2>&1 &
WATCHDOG_PID=$!

sleep 6

kill -TERM "$WATCHDOG_PID"
wait "$WATCHDOG_PID"

if grep -q "exited with code 0" test_normal.txt &&
   grep -q "finished normally, not restarting" test_normal.txt &&
   ! grep -q "restarting child (mode normal)" test_normal.txt; then
    echo "PASS: Normal termination"
else
    FAILED=1
    echo "FAIL: Normal termination"
fi

rm -f tests/process_manager_test.c watchdog_test

echo "===== Test 4: SIGUSR1 Status Reporting ====="

rm -f watchdog.log

./watchdog > test_signals.txt 2>&1 &
WATCHDOG_PID=$!

sleep 1

kill -USR1 "$WATCHDOG_PID"

sleep 1

if grep -q "Child process status report:" watchdog.log &&
   grep -q "Child 1: PID=" watchdog.log &&
   grep -q "Child 2: PID=" watchdog.log &&
   kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    echo "PASS: SIGUSR1 status reporting"
else
    FAILED=1
    echo "FAIL: SIGUSR1 status reporting"
fi

kill -TERM "$WATCHDOG_PID"
wait "$WATCHDOG_PID"
SHUTDOWN_EXIT=$?

echo "===== Test 5: Graceful Shutdown ====="

if [ "$SHUTDOWN_EXIT" -eq 0 ] &&
   grep -q "Received SIGTERM, initiating graceful shutdown" test_signals.txt &&
   grep -q "Sent SIGTERM to child" test_signals.txt &&
   grep -q "Reaped child" test_signals.txt &&
   grep -q "Graceful shutdown complete" test_signals.txt &&
   ! kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    echo "PASS: Graceful shutdown"
else
    FAILED=1
    echo "FAIL: Graceful shutdown"
fi

echo "===== Test 6: Log Verification ====="

if [ -s watchdog.log ] &&
   ! grep -Evq '^\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] .+' watchdog.log &&
   grep -q "Child process status report:" watchdog.log &&
   grep -q "Received SIGTERM" watchdog.log &&
   grep -q "Graceful shutdown complete" watchdog.log; then
    echo "PASS: Log verification"
else
    FAILED=1
    echo "FAIL: Log verification"
fi

rm -f test_crash.txt test_normal.txt test_signals.txt watchdog.log

echo "===== Test Summary ====="

if [ "$FAILED" -eq 0 ]; then
    echo "All watchdog tests passed."
else
    echo "Some tests failed."
fi

echo "Exit code: $FAILED"
exit "$FAILED"
