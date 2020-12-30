// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <functional>
#include <thread>
#include <vector>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "MockExecutor.h"
#include "MockGRPCWorker.h"

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

  // TODO: add more functions / internal variables to connect with DB.

};  // class FrontendServiceImpl

/*
 * Receive a submitted task from the client.
 */
Status FrontendServiceImpl::SubmitTask(ServerContext* context,
                                       const SubmitTaskRequest* request,
                                       SubmitTaskResponse* reply) {
  // std::cout << "Received a task: " << request->requirement() << ", "
  //           << request->exectime() << "μs." << std::endl;
  // TODO: Call executor_->executeTask here.
  // executor_->executeTask();
  usleep(request->exectime());
  reply->set_status(DbosStatusEnum::SUCCESS);
  return Status::OK;
}

}  // namespace dbos_scheduler

/*
 * Actually start the gRPC server on a port number.
 */
void MockGRPCWorker::RunServer(const std::string& port) {
  std::string addr = "0.0.0.0:" + port;

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
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  workerServer_->Wait();
}

DbosStatus MockGRPCWorker::startServing() {
  // Create an executor.
  // If we move to C++14, can use make_unique.
  executor_ = std::unique_ptr<MockExecutor>(new MockExecutor());

  // Add the Worker to the database.
  std::vector<voltdb::Parameter> parameterTypes(5);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[4] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);

  voltdb::Procedure procedure("InsertSparkWorker", parameterTypes);
  for (int data : workerData_) {
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
  workerAddr = "localhost:" + port;
  while (workerServer_ == NULL) { ; }  // Spin until server is online.

  return true;
}

DbosStatus MockGRPCWorker::endServing() {
  // Clean up data and threads.
  workerServer_->Shutdown();
  workerThread_->join();
  delete workerThread_;
  return true;
}
