#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
INSTALL_PATH="${SCRIPT_DIR}/../third_party/voltdb-client-cpp"

# Install the required packages
sudo apt-get -y install libboost-all-dev

# Clone and build VoltDB
mkdir -p ./temp
cd ./temp
git clone   https://github.com/VoltDB/voltdb-client-cpp
cd voltdb-client-cpp/
make

# Copy headers and libs.a to install path
cp libvoltdbcpp.a "${INSTALL_PATH}/lib"
cp third_party_libs/linux/* "${INSTALL_PATH}/lib"
cp -r include/ "${INSTALL_PATH}/include"

# Show the files
ls $INSTALL_PATH
