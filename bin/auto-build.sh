#!/bin/bash
# Check cmake version
ver=`cmake --version | head -n 1 | awk -F " " '{print $3}'`
ver1=`echo $ver | awk -F . '{print $1}'`
ver2=`echo $ver | awk -F . '{print $2}'`
echo $ver $ver1 $ver2
if [ $ver1 -lt 4 ] && [ $ver2 -lt 13 ]
then
  echo "cmake version should be higher than 3.13"
  exit
fi

cd ../ || exit
# Check nng library
if [ -e "/usr/local/include/nng" ]
then
	echo nng exists
else

# Get nng src
  if ! [ -e "./lib/nng/src" ]
  then
      echo Can not find nng src files.
      echo Git clone nng from Gitee.
      mkdir lib &&
      cd ./lib &&
      git clone https://gitee.com/mirrors/nng.git &&
      cd ..
  fi
# Build nng
	echo Build nng first.
	cd ./lib/nng/ &&
	mkdir build
	cd build &&
	cmake .. &&
	make -j4 &&
	make install
	cd ../../../

fi

# Build targets
if ! [ -e "./build" ]
then
  mkdir build
fi
cd build &&
cmake .. &&
make -j4 &&

echo lutf build complete
