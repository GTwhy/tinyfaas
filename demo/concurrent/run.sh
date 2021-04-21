#!/bin/bash

COUNT=1000

cd ../../build
./lutf &
sleep 1
echo start_lutf done
cd ../demo/concurrent

typeset -a CLIENT_PID
i=0


./add_func  &
echo add_func done.
sleep 1


while (( i < COUNT ))
do
	i=$(( i + 1 ))
	rand_time=$(( RANDOM % 10000 ))
	echo "Starting client $i"
	./client $rand_time &
	eval CLIENT_PID[$i]=$!
done

i=0
while (( i < COUNT ))
do
	let "i++"
	wait ${CLIENT_PID[$i]}
done

echo stop lutf-servers.
chmod +x ../../bin/*
../../bin/stop_worker.sh