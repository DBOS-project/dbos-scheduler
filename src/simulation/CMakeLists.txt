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

# Link to libs.
target_link_libraries(SyntheticWorker
                      simworker-util)

target_link_libraries(SyntheticScheduler
                      lib_scheduler
                      simworker-util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(LoadGenerator
                      lib_scheduler
                      simworker-util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(AsyncSyntheticScheduler
                      lib_scheduler
                      simworker-util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(CommunicationBench
                      lib_scheduler
                      simworker-util
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
                      simworker-util
                      dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

# Generate output to lib/ or bin/
set_target_properties(SyntheticScheduler LoadGenerator AsyncSyntheticScheduler CommunicationBench TestPartitionedFIFOScheduler TestSparkScheduler TestPartitionedScanTask SyntheticWorker
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
