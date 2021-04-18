#!/bin/bash

pid=`ps -al | grep "lutf" | awk -F " " '{print $4}'`
kill -9 $pid
echo "Worker has exited"
