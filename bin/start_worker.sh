#!/bin/bash
echo "$0 <ip> <port> or just run $0 with default parameters"

if ! [ -e "../build/lutf" ]
then
  echo Run auto-build.sh to build targets first.
  exit
else
    cd ../build || exit

    if [ $# -eq 2 ]
    then
      ./lutf "$1" "$2"
    else
      ./lutf
    fi

fi