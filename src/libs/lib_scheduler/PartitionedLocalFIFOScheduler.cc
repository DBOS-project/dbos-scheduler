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

#include "PartitionedLocalFIFOScheduler.h"

static std::atomic<uint32_t> taskindex;

void PartitionedLocalFIFOScheduler::truncateWorkerTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateWorkerTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateWorkerTable procedure failed. " << r.toString();
  }
}

void PartitionedLocalFIFOScheduler::truncateTaskTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateTaskTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateTaskTable procedure failed. " << r.toString();
  }
}

DbosStatus PartitionedLocalFIFOScheduler::insertWorker(DbosId workerID,
                                                  int32_t capacity) {
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);

  voltdb::Procedure procedure("InsertWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerID)
      .addInt32(capacity)
      .addInt32(workerID % workerPartitions_)
      .addString("");
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosId PartitionedLocalFIFOScheduler::selectWorker(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  int activePartitions = std::min(workerPartitions_, numWorkers_);
  int offset = rand() % activePartitions;
  for (int count = 0; count < activePartitions; count++) {
    int partitionNum = (count + offset) % activePartitions;
    voltdb::Procedure procedure("SelectOrderedWorker", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(partitionNum).addInt32(taskID);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "SelectWorker procedure failed. " << r.toString()
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

DbosStatus PartitionedLocalFIFOScheduler::assignTaskToWorker(DbosId taskId,
                                                        DbosId workerId) {
  // TODO: implement the actual transaction here.
  std::cout << "assignTaskToWorker, to be implemented." << std::endl;
  std::cout << taskId << " -> " << workerId << std::endl;
  DbosStatus retStatus = true;

  return retStatus;
}

DbosStatus PartitionedLocalFIFOScheduler::finishTask(DbosId taskId,
                                                DbosId workerId) {
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

DbosStatus PartitionedLocalFIFOScheduler::setup() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  DbosStatus ret;
  for (int i = 0; i < numWorkers_; ++i) {
    ret = insertWorker(i, workerCapacity_);
    if (!ret) { return false; }
  }
  return true;
}

DbosStatus PartitionedLocalFIFOScheduler::teardown() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  return true;
}

DbosStatus PartitionedLocalFIFOScheduler::schedule(Task* task) {
  int taskId = taskindex.fetch_add(1);
  DbosId workerId = selectWorker(taskId);
  assert(workerId >= 0);
  return true;
}

DbosStatus PartitionedLocalFIFOScheduler::asyncSchedule(
    boost::shared_ptr<voltdb::ProcedureCallback> callback) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  int taskID = taskindex.fetch_add(1);
  int activePartitions = std::min(workerPartitions_, numWorkers_);
  int partitionNum = rand() % activePartitions;
  voltdb::Procedure procedure("SelectOrderedWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(partitionNum).addInt32(taskID);
  client_->invoke(procedure, callback);
  // TODO: what if it cannot find a worker? The callback can retry?

  return true;
}
