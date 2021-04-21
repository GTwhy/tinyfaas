#!/bin/bash

pid=`ps -al | grep "client" | awk -F " " '{print $4}'`
kill -9 $pid
echo "Clients has exited"
