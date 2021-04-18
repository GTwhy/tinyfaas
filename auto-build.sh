if [ -e "/usr/local/include/nng" ]
then
	echo nng exists
else

  if ! [ -e "./lib/nng/src" ]
  then
      echo Can not find nng src files.
      echo Git clone nng from Gitee.
      echo Make sure the cmake --version is higher than 3.13.
      cd ./lib &&
      git clone https://gitee.com/mirrors/nng.git &&
      cd ..
  fi

	echo Build nng first.
	cd ./lib/nng/ &&
	mkdir build &&
	cd build &&
	cmake .. &&
	make -j4 &&
	make install
	cd ../../../

fi

mkdir build &&
cd build &&
cmake .. &&
make -j4 &&

echo lutf build complete
