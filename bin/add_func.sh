#!/bin/bash

if [ $# -eq 1 ] && [ "$1" == "-h" ]
then
  echo Usage:
  echo "  $0 <func_server_ip> <func_server_port> <work_server_url>
                    <workload_path> <func_name> <app_id> <func_id>"
  echo "  Or just run $0 with default parameters for show"
  exit
fi

if ! [ -e "../build/add_func" ]
then
  echo Run auto-build.sh to build targets first.
  exit
else
    cd ../build || exit

    if [ $# -eq 7 ]
    then
      ./add_func "$1" "$2" "$3" "$4" "$5" "$6" "$7"
    else
      ./add_func
    fi

fi