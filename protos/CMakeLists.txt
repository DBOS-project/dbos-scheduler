message(STATUS "Building DBOS scheduler protos")

INCLUDE(GRPCGenerateCPP)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
  dbos_status.proto
  frontend.proto
  scheduler.proto
  ipc_bench.proto  
)

PROTOBUF_GENERATE_GRPC_CPP(PROTO_GRPC_SRCS PROTO_GRPC_HDRS
  frontend.proto
  scheduler.proto
  ipc_bench.proto
)

add_library(dbos-scheduler-protos
  ${PROTO_SRCS}
  ${PROTO_GRPC_SRCS}
)

target_include_directories(dbos-scheduler-protos PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

# For IPC benchmark.
PROTOBUF_GENERATE_CPP(IPC_SRCS IPC_HDRS
  ipc_bench.proto  
)

PROTOBUF_GENERATE_GRPC_CPP(IPC_GRPC_SRCS IPC_GRPC_HDRS
  ipc_bench.proto
)

add_library(ipcbench-protos
  ${IPC_SRCS}
  ${IPC_GRPC_SRCS}
)

target_include_directories(ipcbench-protos PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

