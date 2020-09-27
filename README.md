# dbos-scheduler

## Pre-requisite
Install cmake-3.13, gRPC-v1.32, and VoltDB:
- `./scripts/install_cmake.sh`
- `./scripts/install_grpc.sh`
- `./scripts/install_voltdb.sh INSTALLATION_PATH`

## Build
```
mkdir build/
cd build/
cmake ..
make -j8
```
