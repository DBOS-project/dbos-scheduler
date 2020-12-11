message(STATUS "Building DBOS scheduler src")

# Packages
set(protobuf_MODULE_COMPATIBLE TRUE)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(_VOLTDB_LIBS_PATH "${CMAKE_SOURCE_DIR}/third_party/voltdb-client-cpp/lib")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/third_party")

include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)

message(STATUS "Using gRPC ${gRPC_VERSION}")

set(PROTOS_PATH "${CMAKE_CURRENT_BINARY_DIR}/../protos")

include_directories(${PROTOS_PATH})

# Subdirectories
add_subdirectory(simulation)
