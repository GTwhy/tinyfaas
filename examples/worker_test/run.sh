#!/bin/bash
#Use dos2unix to translate this script before use.
#Because Win and UNIX have different definitions of newline characters.

FUNC_ADDR="ipc:///tmp/func_demo"
WORK_ADDR="ipc:///tmp/work_demo"

./lutf $FUNC_ADDR &
SERVER_PID=$!
# shellcheck disable=SC2064
trap "kill $SERVER_PID" 0
./worker_test $FUNC_ADDR $WORK_ADDR

kill $SERVER_PID