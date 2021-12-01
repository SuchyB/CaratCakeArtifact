#!/bin/bash

export CC=clang
export CPP=clang++
export CXX=clang++

rm -rf build/ ; 
mkdir build ; 
cd build ; 
cmake3 -DCMAKE_INSTALL_PREFIX="~/CAT" -DCMAKE_BUILD_TYPE=Debug ../ ; 
make -j ;
make install ;
cd ../ 
