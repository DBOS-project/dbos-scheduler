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

DbosStatus SparkScheduler::insertWorker(DbosId workerID, int32_t capacity,
                                        std::vector<int32_t> workerData) {
  VoltdbWorkerUtil* worker = new MockGRPCWorker(client_, workerID, workerPartitions_, capacity, workerData);
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
  while(iterator.hasNext()) {
    voltdb::Row prow = iterator.next();
    DbosId partitionNum = prow.getInt64(0);
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

DbosStatus SparkScheduler::assignTaskToWorker(DbosId taskId, DbosId workerId) {
  // TODO:  Actually lookup the address somehow.
  const std::string& port = std::to_string(8000 + workerId);
  std::string workerAddr = "localhost:" + port;

  // Create stub
  std::shared_ptr<Channel> channel =
      grpc::CreateChannel(workerAddr, grpc::InsecureChannelCredentials());
  std::unique_ptr<dbos_scheduler::Frontend::Stub> stub =
      dbos_scheduler::Frontend::NewStub(channel);

  // Submit task
  dbos_scheduler::SubmitTaskRequest st_request;
  st_request.set_requirement(10);
  st_request.set_exectime(1000);
  dbos_scheduler::SubmitTaskResponse st_reply;

  ClientContext st_context;
  Status status;
  std::unique_ptr<ClientAsyncResponseReader<dbos_scheduler::SubmitTaskResponse>> rpc(stub->PrepareAsyncSubmitTask(&st_context, st_request, &cq));

  rpc->StartCall();
  rpc->Finish(&st_reply, &status, (void*) 1);

  void* got_tag;
  bool ok = false;
  cq.Next(&got_tag, &ok);

  assert(got_tag == (void*) 1);
  return ok;
}

DbosStatus SparkScheduler::finishTask(DbosId taskId, DbosId workerId) {
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("FinishWorkerTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerId).addInt32(-1).addInt32(workerId % workerPartitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "assignTaskToWorker procedure failed. " << r.toString();
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
  return true;
}


DbosStatus SparkScheduler::schedule() {
  DbosId targetData = rand() % (numWorkers_);
  DbosId workerId = selectWorker(targetData);
  assert(workerId >= 0);
  return assignTaskToWorker(0, workerId);
}
