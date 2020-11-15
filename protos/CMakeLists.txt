message(STATUS "Building DBOS scheduler protos")

INCLUDE(GRPCGenerateCPP)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
  dbos_status.proto
  frontend.proto
  scheduler.proto
)

PROTOBUF_GENERATE_GRPC_CPP(PROTO_GRPC_SRCS PROTO_GRPC_HDRS
  frontend.proto
  scheduler.proto
)

add_library(dbos-scheduler-protos
  ${PROTO_SRCS}
  ${PROTO_GRPC_SRCS}
)

target_include_directories(dbos-scheduler-protos PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

# TODO: how to make sure GRPC is compiled before this target?
#add_dependencies(dbos-scheduler-protos ...)
