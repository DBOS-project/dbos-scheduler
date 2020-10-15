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

#include "simulation/PartitionedFIFOTaskScheduler.h"

#define SUCCESS 0
#define NOTASK -1
#define NOWORKER -2

void PartitionedFIFOTaskScheduler::truncateWorkerTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateWorkerTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateWorkerTable procedure failed. " << r.toString();
  }
}

void PartitionedFIFOTaskScheduler::truncateTaskTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateTaskTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateTaskTable procedure failed. " << r.toString();
  }
}

DbosStatus PartitionedFIFOTaskScheduler::insertWorker(DbosId workerID,
                                                  int32_t capacity) {
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerID).addInt32(capacity).addInt32(workerID %
                                                         partitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus PartitionedFIFOTaskScheduler::insertTask(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(taskID).addInt32(-1).addInt32(1).addInt32(taskID %
                                                             partitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus PartitionedFIFOTaskScheduler::selectTaskWorker() {
  std::vector<voltdb::Parameter> parameterTypes(1);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  // Actual num partitions.
  int activePartitions = std::min(partitions_, std::max(numWorkers_, numTasks_));
  int pkey = rand() % activePartitions;

  // Throw a coin and decide whether to use single partition transaction.
  float coin = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

  if (coin > probMultiTx_) {
    // Try to find a task and a worker in a single partition.
    for (int count = 1; count <= activePartitions; ++count) {
      voltdb::Procedure procedure("SelectPartitionedTaskWorker", parameterTypes);
      voltdb::ParameterSet* params = procedure.params();
      params->addInt32(pkey);
      voltdb::InvocationResponse r = client_->invoke(procedure);
      if (r.failure()) {
        std::cout << "SelectPartitionedTaskWorker procedure failed. " << r.toString()
                  << std::endl;
        return false;
      }
      std::vector<voltdb::Table> results = r.results();
      voltdb::Row row = results[0].iterator().next();
      int status = row.getInt64(0);
      if (status == SUCCESS) { return true; }
      else if (status == NOWORKER) {
        // If there is no worker, we need to check all partitions.
        break;
      } else {
        // If there is no task in this partition, we need to change partition.
        pkey = (count + pkey) % activePartitions;
      }
    }
  }

  // Check all partitions.
  parameterTypes.clear();
  voltdb::Procedure naive_procedure("SelectTaskWorker", parameterTypes);
  voltdb::InvocationResponse r = client_->invoke(naive_procedure);
  if (r.failure()) {
    std::cout << "SelectWorker procedure failed. " << r.toString()
              << std::endl;
    return false;
  }
  std::vector<voltdb::Table> results = r.results();
  voltdb::Row row = results[0].iterator().next();
  int status = row.getInt64(0);
  if (status == SUCCESS) { return true; }
  return false;
}

DbosStatus PartitionedFIFOTaskScheduler::setup() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  DbosStatus ret;
  std::cout << "Foo" << std::endl;
  for (int i = 0; i < numWorkers_; ++i) {
    ret = insertWorker(i, workerCapacity_);
    if (!ret) { return false; }
  }
  std::cout << "Foo" << std::endl;
  for (int i = 0; i < numTasks_; ++i) {
    ret = insertTask(i);
    if (!ret) { return false; }
  }
  return true;
}

DbosStatus PartitionedFIFOTaskScheduler::teardown() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  return true;
}

DbosStatus PartitionedFIFOTaskScheduler::schedule() {
  DbosStatus status = selectTaskWorker();
  assert(status >= 0);
  return true;
}
