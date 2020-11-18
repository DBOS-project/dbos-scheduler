// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/SparkScheduler.h"

void SparkScheduler::truncateWorkerTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateWorkerTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateWorkerTable procedure failed. " << r.toString();
  }
}

std::vector<VoltdbWorkerUtil*> SparkScheduler::workers_;

DbosStatus SparkScheduler::insertWorker(DbosId workerID, int32_t capacity,
                                        std::vector<int32_t> workerData) {
  VoltdbWorkerUtil* worker = new MockGRPCWorker(client_, workerID, workerPartitions_, capacity, workerData);
  SparkScheduler::workers_.push_back(worker);
  return worker->setup();
}

DbosId SparkScheduler::selectWorker(DbosId targetData) {
  // TODO: implement the actual transaction here.
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  std::vector<voltdb::Parameter> partitionParameterTypes(1);
  partitionParameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  voltdb::Procedure procedure("SelectDataShardPartition", partitionParameterTypes);
  voltdb::ParameterSet* pparams = procedure.params();
  pparams->addInt32(targetData);
  voltdb::InvocationResponse pr = client_->invoke(procedure);
  std::vector<voltdb::Table> presults = pr.results();
  voltdb::TableIterator iterator = presults[0].iterator();
  std::vector<int> targetPartitions;
  while(iterator.hasNext()) {
    voltdb::Row prow = iterator.next();
    DbosId partitionNum = prow.getInt64(0);
    targetPartitions.push_back(partitionNum);
  }
  // Poll until a slot is found, randomly selecting partitions.
  while(true) { // TODO:  Add timeout.
    int partitionNum = targetPartitions.at(rand() % targetPartitions.size());
    voltdb::Procedure procedure("SelectSparkWorker", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(partitionNum);
    params->addInt32(targetData);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "SelectSparkWorker procedure failed. " << r.toString()
                << std::endl;
      return -1;
    }
    std::vector<voltdb::Table> results = r.results();
    voltdb::Row row = results[0].iterator().next();
    DbosId selectedWorker = row.getInt64(0);
    if (selectedWorker != -1) { return selectedWorker; }
  }
  return -1;
}

std::shared_ptr<Channel> SparkScheduler::addrToChannel(std::string workerAddr) {
  auto got = channelMap.find(workerAddr);
  if (got != channelMap.end()) {
    return got->second;
  } else {
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(workerAddr, grpc::InsecureChannelCredentials());
    channelMap.emplace(workerAddr, channel);
    return channel;
  }
}

DbosStatus SparkScheduler::assignTaskToWorker(DbosId taskId, DbosId workerId) {
  // TODO:  Actually lookup the address somehow.
  const std::string& port = std::to_string(8000 + workerId);
  std::string workerAddr = "localhost:" + port;

  // Create stub
  std::shared_ptr<Channel> channel = addrToChannel(workerAddr);
  std::unique_ptr<dbos_scheduler::Frontend::Stub> stub =
      dbos_scheduler::Frontend::NewStub(channel);

  // Submit task
  dbos_scheduler::SubmitTaskRequest st_request;
  st_request.set_requirement(10);
  st_request.set_exectime(1000);

  // Call object to store rpc data
  AsyncClientCall* call = new AsyncClientCall;
  call->workerID = workerId;
  // stub_->AsyncSubmitTask() performs the RPC call, returning an instance to
  // store in "call". Because we are using the asynchronous API, we need to
  // hold on to the "call" instance in order to get updates on the ongoing RPC.
  call->response_reader = stub->AsyncSubmitTask(&call->context, st_request, &cq_);
  // Request that, upon completion of the RPC, "reply" be updated with the
  // server's response; "status" with the indication of whether the operation
  // was successful. Tag the request with the memory address of the call object.
  call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  
  return true;
}

void SparkScheduler::finishRequests() {
  void* got_tag;
  bool ok = false;
  // TODO:  Don't hardcode.
  voltdb::Client client =
    VoltdbSchedulerUtil::createVoltdbClient("testuser", "testpassword");
  client.createConnection("localhost");
  // Block until the next result is available in the completion queue "cq".
  while (cq_.Next(&got_tag, &ok)) {
    // The tag in this example is the memory location of the call object
    AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);
    assert(ok);
    assert(call->status.ok());
    finishTask(client, -1, call->workerID);
    delete call;
  }
  client.close();
}

DbosStatus SparkScheduler::finishTask(voltdb::Client client, DbosId taskId, DbosId workerId) {
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("FinishWorkerTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerId).addInt32(taskId).addInt32(workerId % workerPartitions_);
  voltdb::InvocationResponse r = client.invoke(procedure);
  if (r.failure()) {
    std::cout << "finishTask procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus SparkScheduler::setup() {
  // Clean up data from previous run.
  truncateWorkerTable();
  DbosStatus ret;
  int numReplicas = std::min(numWorkers_, 3);
  for (int i = 0; i < numWorkers_; ++i) {
      std::vector<int> t {i};
      ret = insertWorker(i, workerCapacity_, t);
      if (!ret) { return false; }
  }
  return true;
}

DbosStatus SparkScheduler::teardown() {
  // Clean up data from previous run.
  truncateWorkerTable();
  for (VoltdbWorkerUtil* worker: SparkScheduler::workers_) {
    worker->teardown();
  }
  return true;
}


DbosStatus SparkScheduler::schedule() {
  // Create the thread that completes asynchronous requests.
  if (finishRequestsThread_ == NULL) {
    finishRequestsThread_ = new std::thread(&SparkScheduler::finishRequests, this);
  }

  int taskID = taskIDs++;
  DbosId targetData = rand() % (numWorkers_);
  DbosId workerId = selectWorker(targetData);
  assert(workerId >= 0);
  return assignTaskToWorker(taskID, workerId);
}
