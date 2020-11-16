// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include <thread>
#include <functional>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/MockGRPCWorker.h"

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
void MockGRPCWorker::RunServer(const std::string& port) {
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
  builder.SetMaxMessageSize(INT32_MAX);

  // Finally, assemble the server.
  workerServer_ = builder.BuildAndStart();
  std::cout << "Frontend server listening on: " << addr << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  workerServer_->Wait();
}


DbosStatus MockGRPCWorker::setup() {
  // Add the Worker to the database.
  std::vector<voltdb::Parameter> parameterTypes(5);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[4] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);

  voltdb::Procedure procedure("InsertSparkWorker", parameterTypes);
  for (int data: workerData_) {
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(workerId_)
        .addInt32(capacity_)
        .addInt32(data)
        .addInt32(workerId_ % workerPartitions_)
        .addString("");
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "InsertWorker procedure failed. " << r.toString();
      return false;
    }
  }

  const std::string& port = std::to_string(8000 + workerId_);
  workerThread_ = new std::thread(&MockGRPCWorker::RunServer, this, port);

  // TESTING
  sleep(1);
  std::string addr = "localhost:" + port;
  std::cout << "Contacting frontend server at address: " << addr << std::endl;
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
  // TESTING

  return true;
}

DbosStatus MockGRPCWorker::teardown() {
  // Clean up data and threads.
  std::cout << "Stop worker " << workerId_ << std::endl;
  workerServer_->Shutdown();
  workerThread_->join();
  delete workerThread_;
  return true;
}
