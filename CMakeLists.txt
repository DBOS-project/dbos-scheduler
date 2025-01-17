cmake_minimum_required(VERSION 3.5)
message(STATUS "Using CMake version ${CMAKE_VERSION}")
project(dbos-scheduler)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Additional CMake files path
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(gRPC_SSL_PROVIDER "package" CACHE STRING "")

# NOTE: change this path or comment it out if openssl is installed at a different path.
set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl/install" CACHE STRING "")
set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl/install/include" CACHE STRING "")
find_package(OpenSSL REQUIRED)

# Find gRPC installation
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_GRPC_GRPCPP grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc/include)

# Find protobuf installation
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/cmake/install/)
INCLUDE(FindProtobuf)
find_package(Protobuf CONFIG REQUIRED)
find_program(Protobuf_PROTOC_EXECUTABLE protoc)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Targets
add_subdirectory(protos)
add_subdirectory(src)

