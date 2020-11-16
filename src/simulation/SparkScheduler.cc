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
  workers_.push_back(worker);
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
  while(presults[0].iterator().hasNext()) {
    voltdb::Row prow = presults[0].iterator().next();
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
  // TODO: implement the actual transaction here.
  std::cout << "assignTaskToWorker, to be implemented." << std::endl;
  std::cout << taskId << " -> " << workerId << std::endl;
  DbosStatus retStatus = true;

  return retStatus;
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
  for (VoltdbWorkerUtil* worker: workers_) {
    worker->teardown();
  }
  truncateWorkerTable();
  return true;
}

DbosStatus SparkScheduler::schedule() {
  DbosId targetData = rand() % (numWorkers_);
  DbosId workerId = selectWorker(targetData);
  assert(workerId >= 0);
  return true;
}
