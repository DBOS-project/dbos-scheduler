message(STATUS "Building DBOS scheduler src")

# Packages
set(protobuf_MODULE_COMPATIBLE TRUE)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories("${CMAKE_SOURCE_DIR}/src")

include_directories(/usr/local/include)
link_directories(/usr/local/lib /usr/local/lib64)


if(BUILD_WITH_GRPC)
  find_package(Protobuf REQUIRED)
  find_package(gRPC CONFIG REQUIRED)
  message(STATUS "Using gRPC ${gRPC_VERSION}")
  find_package(OpenSSL REQUIRED)

  set(PROTOS_PATH "${CMAKE_CURRENT_BINARY_DIR}/../protos")
  set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
  set(_REFLECTION gRPC::grpc++_reflection)
  set(_GRPC_GRPCPP gRPC::grpc++)

  include_directories(${PROTOS_PATH})

  add_executable(frontend_grpc_server frontend_grpc_server.cc)
  target_link_libraries(frontend_grpc_server dbos-scheduler-protos
                        ${_REFLECTION}
                        ${_GRPC_GRPCPP}
                        ${_PROTOBUF_LIBPROTOBUF}
                        pthread
  )

  set_target_properties(frontend_grpc_server
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
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
endif(BUILD_WITH_GRPC)
