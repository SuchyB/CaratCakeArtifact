#! /usr/bin/sh

# For peroni
TC=../toolchain
source /project/extra/llvm/9.0.0/enable
nwd=$(pwd)

# Copy the source into the toolchain for build
cd ./src/llvm/$1/

cp ./driver/$2 $TC/catpass/CatPass.cpp
cp ./CMakeLists.txt $TC/catpass/
cp -ar ./src/ $TC/catpass/
cp -ar ./include/ $TC/catpass/

# Build from the toolchain
cd $TC 
./run_me.sh

cd $nwd # Nautilus working directory --- very suspicious
