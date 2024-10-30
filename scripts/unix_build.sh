#!/bin/sh
git submodule update --init
cd ./third-party/pq
./configure with_icu=no --prefix=$(pwd)/build
make -j && make install
cd ../..
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j