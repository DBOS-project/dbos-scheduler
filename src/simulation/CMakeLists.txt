message(STATUS "Building DBOS scheduler src/simulation")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

# Add executable files.
add_executable(SyntheticScheduler SyntheticScheduler.cc)
add_executable(LoadGenerator LoadGenerator.cc)
add_executable(AsyncSyntheticScheduler AsyncSyntheticScheduler.cc)
add_executable(CommunicationBench CommunicationBench.cc)
add_executable(TestPartitionedFIFOScheduler TestPartitionedFIFOScheduler.cc)
add_executable(TestSparkScheduler TestSparkScheduler.cc)
add_executable(TestPartitionedScanTask TestPartitionedScanTask.cc)
add_executable(SyntheticWorker SyntheticWorker.cc)
add_executable(TCPBenchClient TCPBenchClient.cc)
add_executable(TCPBenchServer TCPBenchServer.cc)
add_executable(DBCommBenchClient DBCommBenchClient.cc)
add_executable(DBCommBenchServer DBCommBenchServer.cc)
add_executable(GrpcBenchServer GrpcBenchServer.cc)
add_executable(GrpcBenchClient GrpcBenchClient.cc)

# Link to libs.
target_link_libraries(GrpcBenchServer
                      ipcbench-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(GrpcBenchClient
                      ipcbench-protos
                      lib_util
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(DBCommBenchClient
                      lib_util
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl)

target_link_libraries(DBCommBenchServer
                      lib_util
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl)

target_link_libraries(TCPBenchClient
                      lib_util
                      pthread)

target_link_libraries(TCPBenchServer
                      pthread)

target_link_libraries(SyntheticWorker
                      lib_worker
                      lib_util)

target_link_libraries(SyntheticScheduler
                      lib_scheduler
                      lib_worker
                      lib_util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(LoadGenerator
                      lib_scheduler
                      lib_worker
                      lib_util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(AsyncSyntheticScheduler
                      lib_scheduler
                      lib_worker
                      lib_util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(CommunicationBench
                      lib_scheduler
                      lib_worker
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(TestPartitionedScanTask
                      lib_scheduler
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(TestPartitionedFIFOScheduler
                      lib_scheduler
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(TestSparkScheduler 
                      lib_scheduler
                      lib_worker
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

# Generate output to lib/ or bin/
set_target_properties(SyntheticScheduler LoadGenerator AsyncSyntheticScheduler CommunicationBench TestPartitionedFIFOScheduler TestSparkScheduler TestPartitionedScanTask SyntheticWorker TCPBenchClient TCPBenchServer DBCommBenchClient DBCommBenchServer GrpcBenchServer GrpcBenchClient
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
