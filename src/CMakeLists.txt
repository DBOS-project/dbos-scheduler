message(STATUS "Building DBOS scheduler src")

# Packages
find_package(Protobuf REQUIRED)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(PROTOS_PATH "${CMAKE_CURRENT_BINARY_DIR}/../protos")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories(${PROTOS_PATH})

include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)
