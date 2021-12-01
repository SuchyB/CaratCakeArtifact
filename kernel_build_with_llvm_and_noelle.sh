#!/usr/bin/env -S bash -x

# To build the CARAT CAKE LLVM passes and the integrated kernel,
# run this script at the top level Nautilus directory

NAUT=`pwd`

# Install LLVM 9.0.0
LLVM_TOP_DIR=${NAUT}/9.0.0
git clone https://github.com/sgh185/LLVM_installer.git ${LLVM_TOP_DIR} ;
cd ${LLVM_TOP_DIR} && make && cd ${NAUT} ; 

# Use the newly installed LLVM
source ${LLVM_TOP_DIR}/enable ;

# Install Noelle
NOELLE_TOP_DIR=${NAUT}/noelle
git clone https://github.com/scampanoni/noelle.git ${NOELLE_TOP_DIR} ;
cd ${NOELLE_TOP_DIR} && make && cd ${NAUT} ;

# Use the newly installed Noelle
source ${NOELLE_TOP_DIR}/enable ;

# Set up Nautilus configurations
make clean ; make include/autoconf.h ; 

# Pass build
./scripts/pass_build.sh carat KARAT.cpp ;

# Kernel build
make -j ; make bitcode ; make karat_noelle ; make final ; make isoimage ;
