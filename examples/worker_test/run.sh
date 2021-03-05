#!/bin/bash

FUNC_ADDR="ipc:///tmp/func_demo"
WORK_ADDR="ipc:///tmp/work_demo"

./../../cmake-build-debug/lutf $FUNC_ADDR &
SERVER_PID=$!
# shellcheck disable=SC2064
trap "kill $SERVER_PID" 0
./../../cmake-build-debug/worker_test $FUNC_ADDR $WORK_ADDR

kill $SERVER_PID