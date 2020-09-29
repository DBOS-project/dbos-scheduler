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
set(simulation-util_SOURCES VoltdbClientUtil.cc BenchmarkUtil.cc)
add_library(simulation-util STATIC ${simulation-util_SOURCES})

target_link_libraries(simulation-util
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
add_executable(TestVoltdbClientUtil TestVoltdbClientUtil.cc)

# Link to libs.
target_link_libraries(SyntheticScheduler
                      simulation-util
)

target_link_libraries(TestVoltdbClientUtil
                      simulation-util
)

# Generate output to lib/ or bin/
set_target_properties(simulation-util SyntheticScheduler TestVoltdbClientUtil
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
