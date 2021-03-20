
if ! [ -e "./lib/nng/src" ]
then
    echo Can not find nng src files.
    echo Please download nng firstly, and use nng as the top-level directory.
    echo And make sure the cmake --version is higher than 3.13.
    exit 1
fi

if [ -e "/usr/local/include/nng" ]
then
	echo nng exists
else
	echo going to build nng first.
	cd ./lib/nng/ &&
	mkdir build &&
	cd build &&
	cmake .. &&
	make -j4 &&
	make install 
fi

cd ../../../ &&
mkdir build &&
cd build &&
cmake .. &&
make -j4 &&

echo lutf build complete
