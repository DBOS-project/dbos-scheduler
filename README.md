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
For example: `./install_voltdb.sh /home/`. Then VoltDB will be installed under `/home/voltdb`

(Optional for now) Install CMake-3.17
```
./scripts/install_cmake.sh
```

## Init and update submodules
Pull remote grpc and protobuf codebase as third party dependency.
```
git submodule update --init --recursive --progress
```

Then build protobuf:
```
cd third_party/protobuf
mkdir -p ./cmake/build
mkdir -p ./cmake/install
cd cmake/build
cmake -DCMAKE_INSTALL_PREFIX=${PWD}/../install ..
make -j40
make install
```

Then build openssl:
```
cd third_party/openssl
mkdir -p ./install
./config --prefix=${PWD}/install
make -j40
make install
```

## Build DBOS scheduler
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

## Initialize VoltDB and load Stored Procedures
Note that you will need to export "$VOLT_HOME" variable to the VoltDB installation path.
For example `export VOLT_HOME=/home/voltdb`.

Then you could initialize the tables and procedures:
```
./scripts/initialize_voltdb.sh
```

## Shutdown VoltDB
On a single node cluster, run:
```
voltadmin shutdown
```

## Initialize a cluster of DB
```
First, follow the instruction in shared_scripts to init and start a cluster.
Then, on one of the host node, run
./scripts/create_tables.sh
./scripts/update_voltdb_procedures.sh
```
TODO: figure out a way to automate the process. For now, we need to manually
start the cluster because `$TMPDIR` is only available inside a slurm session.
