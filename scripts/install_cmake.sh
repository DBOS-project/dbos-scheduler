#!/bin/bash
set -xe

# Install a specific version of cmake
sudo apt remove --purge --auto-remove cmake
sudo apt update && sudo apt upgrade
sudo apt install libssl-dev

version=3.17
build=0
mkdir ./temp
cd ./temp
wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz
tar -xzvf cmake-$version.$build.tar.gz
pushd cmake-$version.$build/

./bootstrap
make -j8
sudo make install -j8

cmake --version

# Clean up
popd
cd ../
rm -rf ./temp
