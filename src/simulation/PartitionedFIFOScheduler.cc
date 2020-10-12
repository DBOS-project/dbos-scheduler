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

#include "simulation/PartitionedFIFOScheduler.h"

void PartitionedFIFOScheduler::truncateWorkerTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateWorkerTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateWorkerTable procedure failed. " << r.toString();
  }
}

DbosStatus PartitionedFIFOScheduler::insertWorker(DbosId workerID, int32_t capacity) {
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerID).addInt32(capacity).addInt32(workerID % workerPartitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosId PartitionedFIFOScheduler::selectWorker() {
  // TODO: implement the actual transaction here.
  std::vector<voltdb::Parameter> parameterTypes(1);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  int offset = rand() % std::min(workerPartitions_, numWorkers_);
  for (int count = 0; count < workerPartitions_; count++) {
    int partitionNum = (count + offset) % workerPartitions_;
    voltdb::Procedure procedure("SelectWorker", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(partitionNum);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "SelectWorker procedure failed. " << r.toString() << std::endl;
      return -1;
    }
    std::vector<voltdb::Table> results = r.results();
    voltdb::Row row = results[0].iterator().next();
    DbosId selectedWorker = row.getInt64(0);
    if (selectedWorker != -1) {
      return selectedWorker;
    }
  }
  return -1;
}

DbosStatus PartitionedFIFOScheduler::assignTaskToWorker(DbosId taskId,
                                                        DbosId workerId) {
  // TODO: implement the actual transaction here.
  std::cout << "assignTaskToWorker, to be implemented." << std::endl;
  std::cout << taskId << " -> " << workerId << std::endl;
  DbosStatus retStatus = true;

  return retStatus;
}

DbosStatus PartitionedFIFOScheduler::finishTask(DbosId taskId, DbosId workerId) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("FinishWorkerTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerId).addInt32(workerId % workerPartitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "assignTaskToWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus PartitionedFIFOScheduler::setup() {
  // Clean up data from previous run.
  truncateWorkerTable();
  DbosStatus ret;
  for (int i = 0; i < numWorkers_; ++i) {
    ret = insertWorker(i, workerCapacity_);
    if (!ret) { return false; }
  }
  return true;
}

DbosStatus PartitionedFIFOScheduler::teardown() {
  // Clean up data from previous run.
  truncateWorkerTable();
  return true;
}

DbosStatus PartitionedFIFOScheduler::schedule() {
  DbosId workerId = selectWorker();
  assert(workerId >= 0);
  return true;
}
