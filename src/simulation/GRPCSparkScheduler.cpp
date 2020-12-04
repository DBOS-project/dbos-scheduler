#include "simulation/GRPCSparkscheduler.h"

namespace dbos_scheduler {

// Implement the frontend gRPC service here.
class FrontendServiceGRPCSparkScheduler final : public Frontend::Service {
public:
  FrontendServiceGRPCSparkScheduler() {
    // TODO: to be implemented. e.g., initialize connections to VoltDB.
  }

  ~FrontendServiceGRPCSparkScheduler() {
    // TODO: to be implemented.
  }

private:
  Status SubmitTask(ServerContext* context, const SubmitTaskRequest* request,
                    SubmitTaskResponse* reply) override;

  Status Heartbeat(ServerContext* context, const HeartbeatRequest* request,
                   HeartbeatResponse* reply) override;

};  // class FrontendServiceImpl

/*
 * Simply print the message and return.
 */
Status FrontendServiceGRPCSparkScheduler::Heartbeat(ServerContext* context,
                                      const HeartbeatRequest* request,
                                      HeartbeatResponse* reply) {
  std::cout << "Received heartbeat msg: " << request->msg() << std::endl;
  reply->set_status(DbosStatusEnum::SUCCESS);
  return Status::OK;
}

/*
 * Receive a submitted task from the client.
 */
Status FrontendServiceGRPCSparkScheduler::SubmitTask(ServerContext* context, const SubmitTaskRequest* request,
                                       SubmitTaskResponse* reply) {
  // std::cout << "Recieved a task: " << request->requirement() << ", "
  //           << request->exectime() << "μs." << std::endl;

  usleep(request->exectime());
  reply->set_status(DbosStatusEnum::SUCCESS);
  return Status::OK;
}

}  // namespace dbos_scheduler

/*
 * Actually start the gRPC server on a port number.
 */
void GRPCSparkScheduler::RunServer() {
  const std::string& port = std::to_string(port_);
  std::string addr = "0.0.0.0:" + port;

  dbos_scheduler::FrontendServiceGRPCSparkScheduler service;
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Set max message size.
  builder.SetMaxMessageSize(INT32_MAX);

  // Finally, assemble the server.
  workerServer_ = builder.BuildAndStart();
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  workerServer_->Wait();
}

DbosStatus GRPCSparkScheduler::setup() {
  workerThread_ = new std::thread(&GRPCSparkScheduler::RunServer, this);
  while (workerServer_ == NULL) {;} // Spin until server is online.

  return true;
}

DbosStatus GRPCSparkScheduler::teardown() {
  // Clean up data and threads.
  workerServer_->Shutdown();
  workerThread_->join();
  delete workerThread_;
  return true;
}
