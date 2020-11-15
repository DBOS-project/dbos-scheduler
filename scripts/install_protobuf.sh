#!/bin/bash
set -ex

# Script to install protobuf
PROTO_DIR=./third_party/protobuf
cd ${PROTO_DIR}
git submodule update --init --recursive

mkdir -p ./cmake/build
mkdir -p ./cmake/install
cd cmake/build
cmake -DCMAKE_INSTALL_PREFIX=${PWD}/../install ..
make -j40
make install
