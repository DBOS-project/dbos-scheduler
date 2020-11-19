#!/bin/bash
set -ex

# Script to install openssl
OPENSSL_DIR=./third_party/openssl
cd ${OPENSSL_DIR}

mkdir -p ./install
./config --prefix=${PWD}/install
make -j40
make install
