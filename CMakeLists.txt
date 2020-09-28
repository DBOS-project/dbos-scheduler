cmake_minimum_required(VERSION 3.5)
message(STATUS "Using CMake version ${CMAKE_VERSION}")
project(dbos-scheduler)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build with gRPC
option(BUILD_WITH_GRPC "Build with gRPC support." OFF)

# Additional CMake files path
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Dependencies
# Find boost library
find_package(Boost 1.53)
message(STATUS "Using Boost ${Boost_VERSION}")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# If build with gRPC, find protobuf and gRPC installation.
if(BUILD_WITH_GRPC)
  # Find Protobuf installation
  # Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
  set(protobuf_MODULE_COMPATIBLE TRUE)
  find_package(Protobuf CONFIG REQUIRED)

  # Find gRPC installation
  # Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
  find_package(gRPC CONFIG REQUIRED)

  add_subdirectory(protos)
endif(BUILD_WITH_GRPC)

# Targets
add_subdirectory(src)
add_subdirectory(test)

