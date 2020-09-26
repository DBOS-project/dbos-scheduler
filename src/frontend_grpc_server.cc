// Implement frontend gRPC service.
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "frontend.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Constants
static const int kMaxGrpcMsgSize = INT32_MAX;
static const std::string kDefaultPort = "9001";

namespace dbos_scheduler {

// Implement the frontend gRPC service here.
class FrontendServiceImpl final : public Frontend::Service {
public:
  FrontendServiceImpl() {
    // TODO: to be implemented. e.g., initialize connections to VoltDB.
  }

  ~FrontendServiceImpl() {
    // TODO: to be implemented.
  }

private:
  Status SubmitTask(ServerContext* context, const SubmitTaskRequest* request,
                    SubmitTaskResponse* reply) override;

  Status Heartbeat(ServerContext* context, const HeartbeatRequest* request,
                   HeartbeatResponse* reply) override;

  // TODO: add more functions / internal variables to connect with DB.

};  // class FrontendServiceImpl

/*
 * Simply print the message and return.
 */
Status FrontendServiceImpl::Heartbeat(ServerContext* context,
                                      const HeartbeatRequest* request,
                                      HeartbeatResponse* reply) {
  std::cout << "Received heartbeat msg: " << request->msg() << std::endl;
  reply->set_status(DbosStatusEnum::SUCCESS);
  return Status::OK;
}

/*
 * Receive a submitted task from the client.
 */
Status FrontendServiceImpl::SubmitTask(ServerContext* context, const SubmitTaskRequest* request,
                  SubmitTaskResponse* reply) {
  // TODO: implement the actual logic here.
  std::cout << "Recieved a task: " << request->requirement() << ", "
            << request->exectime() << "ns." << std::endl;

  reply->set_status(DbosStatusEnum::SUCCESS);
  return Status::OK;
}

}  // namespace dbos_scheduler

/*
 * Actually start the gRPC server on a port number.
 */
void RunServer(const std::string& port) {
  std::string addr = "0.0.0.0:" + port;
  std::cout << "Starting frontend server at address: " << addr << std::endl;

  dbos_scheduler::FrontendServiceImpl service;
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Set max message size.
  builder.SetMaxMessageSize(kMaxGrpcMsgSize);

  // Finally, assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Frontend server listening on: " << addr << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  std::string port = kDefaultPort;
  if (argc > 1) {
    port = argv[1];
  } else {
    std::cout << "Usage: " << argv[0] << " <port_num>" << std::endl;
  }

  RunServer(port);
  return 0;
}
