message(STATUS "Building DBOS scheduler lib/lib_scheduler")

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

set(lib_scheduler_SOURCES PartitionedFIFOScheduler.cc PartitionedFIFOTaskScheduler.cc SinglePartitionedFIFOTaskScheduler.cc VoltdbSchedulerUtil.cc BenchmarkUtil.cc SparkScheduler.cc SchedulerServer.cpp PartitionedScanTask.cc PushFIFOScheduler.cc)
add_library(lib_scheduler STATIC ${lib_scheduler_SOURCES})

# For scheduler simulation
target_include_directories(lib_scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_scheduler
                      dbos-scheduler-protos
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl
                      simworker-util
)

set_target_properties(lib_scheduler
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
