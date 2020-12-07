#include "simulation/GRPCSparkScheduler.h"
#include "simulation/SparkScheduler.h"

namespace dbos_scheduler {

// Implement the frontend gRPC service here.
class FrontendServiceGRPCSparkScheduler final : public Frontend::Service {
public:
  FrontendServiceGRPCSparkScheduler(voltdb::Client* client, std::string dbAddr,
                                    int workerPartitions, int workerCapacity, int numWorkers) {
    numWorkers_ = numWorkers;
    sparkScheduler = new SparkScheduler(client, dbAddr, workerPartitions,
                                                   workerCapacity, numWorkers);
  }

  ~FrontendServiceGRPCSparkScheduler() {
    delete sparkScheduler;
  }

private:
  Status SubmitTask(ServerContext* context, const SubmitTaskRequest* request,
                    SubmitTaskResponse* reply) override;

  Status Heartbeat(ServerContext* context, const HeartbeatRequest* request,
                   HeartbeatResponse* reply) override;

  SparkScheduler* sparkScheduler;
  int numWorkers_;

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

  DbosId targetData = rand() % (numWorkers_);
  sparkScheduler->submitTask(targetData);
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

  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient("testuser", "testpassword");

  dbos_scheduler::FrontendServiceGRPCSparkScheduler service(&voltdbClient, dbAddr, workerPartitions, workerCapacity, numWorkers);
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
