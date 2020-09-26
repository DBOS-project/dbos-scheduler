// Test frontend gRPC service.
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "frontend.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// Constants
static const int kMaxGrpcMsgSize = INT32_MAX;
static const std::string kDefaultPort = "9001";

int main(int argc, char** argv) {
  std::string port = kDefaultPort;
  if (argc > 1) {
    port = argv[1];
  } else {
    std::cout << "Usage: " << argv[0] << " <port_num>" << std::endl;
  }
  std::string addr = "localhost:" + port;

  std::shared_ptr<Channel> channel =
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<dbos_scheduler::Frontend::Stub> stub =
      dbos_scheduler::Frontend::NewStub(channel);

  ClientContext context;

  // Test heartbeat.
  dbos_scheduler::HeartbeatRequest request;
  request.set_msg("hello");

  dbos_scheduler::HeartbeatResponse reply;
  Status status = stub->Heartbeat(&context, request, &reply);
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    exit(1);
  } else {
    std::cout << "Heartbeat succeeded!" << std::endl;
  }

  // Test submit task
  dbos_scheduler::SubmitTaskRequest st_request;
  st_request.set_requirement(10);
  st_request.set_exectime(1);
  dbos_scheduler::SubmitTaskResponse st_reply;

  ClientContext st_context;

  status = stub->SubmitTask(&st_context, st_request, &st_reply);
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    exit(1);
  } else {
    std::cout << "SubmitTask succeeded!" << std::endl;
  }

  return 0;
}
