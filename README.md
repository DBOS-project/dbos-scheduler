# dbos-scheduler

## Pre-requisite
Install cmake (at least version 3.5.1):
```
sudo apt-get install cmake
```

For VoltDB C++ client: 
Install Boost C++ Library 1.53 if using the downloaded client library.
If building from source, you may use 1.53 or above.
```
sudo apt-get install libboost-all-dev
```

Install VoltDB:
```
./scripts/install_voltdb.sh INSTALLATION_PATH
```

(Optional for now) Install CMake-3.17 and gRPC-v1.32
```
./scripts/install_cmake.sh
./scripts/install_grpc.sh
```

## Build
```
mkdir build/
cd build/
cmake ..
make -j8
```

(Optional for now) If you would like to build with gRPC:
```
cmake .. -DBUILD_WITH_GRPC=ON
make -j8
```
