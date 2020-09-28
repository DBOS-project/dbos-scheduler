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
set(_VOLTDB_LIBS_PATH "${CMAKE_SOURCE_DIR}/third_party/voltdb-client-cpp/lib")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories(${PROTOS_PATH})
include_directories("${CMAKE_SOURCE_DIR}/third_party/voltdb-client-cpp/include")

include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)

# Add executable files.
add_executable(frontend_grpc_server frontend_grpc_server.cc)
add_executable(voltdb_client_example voltdb_client_example.cc)

# Link to libs.
target_link_libraries(frontend_grpc_server dbos-scheduler-protos
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      pthread
)

target_link_libraries(voltdb_client_example
                      ${_VOLTDB_LIBS_PATH}/libvoltdbcpp.a
                      ${_VOLTDB_LIBS_PATH}/libevent.a
                      ${_VOLTDB_LIBS_PATH}/libevent_openssl.a
                      ${_VOLTDB_LIBS_PATH}/libevent_pthreads.a
                      ${_VOLTDB_LIBS_PATH}/libssl.a
                      ${_VOLTDB_LIBS_PATH}/libcrypto.a
                      pthread boost_system boost_thread dl
)

# Generate output to lib/ or bin/
set_target_properties(frontend_grpc_server voltdb_client_example
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
