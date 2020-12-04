message(STATUS "Building DBOS scheduler src/simulation")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

# Library for internal util.
set(simulation-util_SOURCES PartitionedFIFOScheduler.cc PartitionedFIFOTaskScheduler.cc SinglePartitionedFIFOTaskScheduler.cc VoltdbSchedulerUtil.cc BenchmarkUtil.cc SparkScheduler.cc PartitionedScanTask.cc PushFIFOScheduler.cc)
add_library(simulation-util STATIC ${simulation-util_SOURCES})

set(simworker-util_SOURCES BenchmarkUtil.cc VoltdbWorkerUtil.cc MockPollWorker.cc MockHTTPWorker.cc MockGRPCWorker.cc)
add_library(simworker-util STATIC ${simworker-util_SOURCES})

# For scheduler simulation
target_link_libraries(simulation-util
                      dbos-scheduler-protos
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl
)

# For worker simulation
target_link_libraries(simworker-util
                      dbos-scheduler-protos
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl
)

# Add executable files.
add_executable(SyntheticScheduler SyntheticScheduler.cc)
add_executable(SparkLoadGenerator SparkLoadGenerator.cc)
add_executable(TestPartitionedFIFOScheduler TestPartitionedFIFOScheduler.cc)
add_executable(TestSparkScheduler TestSparkScheduler.cc)
add_executable(TestPartitionedScanTask TestPartitionedScanTask.cc)
add_executable(SyntheticWorker SyntheticWorker.cc)

# Link to libs.
target_link_libraries(SyntheticScheduler
                      simulation-util
)

target_link_libraries(SyntheticScheduler
                      simworker-util
)

target_link_libraries(SparkLoadGenerator
                      simulation-util
)

target_link_libraries(SparkLoadGenerator
                      simworker-util
)

target_link_libraries(SyntheticWorker
                      simworker-util)

target_link_libraries(TestPartitionedFIFOScheduler
                      simulation-util
)

target_link_libraries(TestSparkScheduler
                      simulation-util
)

target_link_libraries(TestSparkScheduler
                      simworker-util
)

target_link_libraries(TestPartitionedScanTask
                      simulation-util)

target_link_libraries(TestSparkScheduler dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(SyntheticScheduler dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(SparkLoadGenerator dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

# Generate output to lib/ or bin/
set_target_properties(simulation-util simworker-util SyntheticScheduler SparkLoadGenerator TestPartitionedFIFOScheduler TestSparkScheduler TestPartitionedScanTask SyntheticWorker
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
