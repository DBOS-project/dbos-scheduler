#!/bin/bash
set -ex

# Script to install gRPC C++
BASE_DIR=./temp/grpc
mkdir -p ./temp
sudo apt-get install -y build-essential autoconf libtool pkg-config

# Check the latest release version at: https://grpc.io/release
# In order to be reproducible, we chose v1.32.0
git clone --recurse-submodules -b v1.32.0 https://github.com/grpc/grpc ${BASE_DIR}
cd ${BASE_DIR}

mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      ../..

make -j8
sudo make install
popd

#rm -rf ${BASE_DIR}

