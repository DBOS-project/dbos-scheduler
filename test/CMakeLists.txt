message(STATUS "Building DBOS scheduler src")

# Packages
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(PROTOS_PATH "${CMAKE_CURRENT_BINARY_DIR}/../protos")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_GRPC_GRPCPP gRPC::grpc++)

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories(${PROTOS_PATH})

include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)

# Add executable files.
add_executable(test_frontend_grpc_server test_frontend_grpc_server.cc)

# Link to libs.
target_link_libraries(test_frontend_grpc_server dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

# Generate output to lib/ or bin/
set_target_properties(test_frontend_grpc_server
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
