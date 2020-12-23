message(STATUS "Building DBOS scheduler libs/lib_worker")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

set(lib_worker_SOURCES Executor.cc WorkerManager.cc MockExecutor.cc MockPollWorker.cc MockHTTPWorker.cc MockGRPCWorker.cc)
add_library(lib_worker STATIC ${lib_worker_SOURCES})

# For worker simulation
target_include_directories(lib_worker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_worker
                      dbos-scheduler-protos
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread Boost::system Boost::thread dl
                      lib_util
)

set_target_properties(lib_worker
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
