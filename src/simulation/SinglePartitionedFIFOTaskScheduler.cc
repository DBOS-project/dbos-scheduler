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

#include "simulation/SinglePartitionedFIFOTaskScheduler.h"

#define SUCCESS 0
#define NOTASK -1
#define NOWORKER -2


#define PROB_FIND_WORKER .95

static std::atomic<uint32_t> taskindex;

void SinglePartitionedFIFOTaskScheduler::truncateWorkerTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateWorkerTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateWorkerTable procedure failed. " << r.toString();
  }
}

void SinglePartitionedFIFOTaskScheduler::truncateTaskTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateTaskTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateTaskTable procedure failed. " << r.toString();
  }
}

DbosStatus SinglePartitionedFIFOTaskScheduler::insertWorker(DbosId workerID,
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
   .addInt32(workerID % partitions_)
   .addString("");
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::insertUnassignedTask(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertUnassignedTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(taskID).addInt32(schedulerId_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertUnassignedTask procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::assignFromTaskQueue() {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  // Actual num partitions.
  int activePartitions =
      std::min(partitions_, numWorkers_);
  int workerPKey = rand() % activePartitions;

  for (int count = 1; count <= activePartitions; ++count) {
    voltdb::Procedure procedure("AssignUnassignedTask", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(workerPKey).addInt32(schedulerId_);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "AssignUnassignedTask procedure failed. " << r.toString();
      return false;
    }
    std::vector<voltdb::Table> results = r.results();
    voltdb::Row row = results[0].iterator().next();
    int status = row.getInt64(0);
    if (status == SUCCESS || status == NOTASK) {
      //if (status == SUCCESS) std::cout << schedulerId_ << " success assigning from TaskQueue." << std::endl;
      //if (status == NOTASK) std::cout << schedulerId_ << " no task in TaskQueue." << std::endl;
  
      return true;
    }
    workerPKey = (workerPKey + count) % activePartitions;
  }
  std::cout << schedulerId_ << " no task in TaskQueue." << std::endl;
  return false;  // No available workers
}

DbosStatus SinglePartitionedFIFOTaskScheduler::findAvailableTaskWorker(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  
  // Actual num partitions.
  int activePartitions = std::min(partitions_, numWorkers_);
  int pkey = rand() % activePartitions;

  // Try to find an available worker in the partition.
  for (int count = 1; count <= activePartitions; ++count) {
    voltdb::Procedure procedure("SelectSinglePartitionedTaskWorker",
                                parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(pkey).addInt32(taskID);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "SelectSinglePartitionedTaskWorker procedure failed. "
                << r.toString() << std::endl;
      return false;
    }
    std::vector<voltdb::Table> results = r.results();
    voltdb::Row row = results[0].iterator().next();
    int status = row.getInt64(0);
    if (status == SUCCESS) {
      return true;
    } else if (status == NOWORKER) {
      std::cout << "no worker in partition " << pkey << std::endl;
    }
    pkey = (pkey + count) % activePartitions;
  }
  return false;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::probabilisticSelectTaskWorker(DbosId taskID) {
  // Throw a coin and decide whether to assign the task to a worker or insert
  // into the unassigned task queue.
  float coin = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  DbosStatus status;
  if (coin < PROB_FIND_WORKER) {  // Find an available worker in the partition.
    status = findAvailableTaskWorker(taskID);
    assert(status == true);
    return true;
  } 
  // Insert into task queue
  status = insertUnassignedTask(taskID);
  assert(status == true);
  return false;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::selectTaskWorker(DbosId taskID) {
 DbosStatus status = findAvailableTaskWorker(taskID);
 if (status) {
    return true;
  }
  // Went through all partititions but wasn't able to find an available worker 
  // or procedure failed; add to unassigned task queue
  return insertUnassignedTask(taskID);
}

DbosStatus SinglePartitionedFIFOTaskScheduler::setup() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  DbosStatus ret;
  std::cout << "Foo" << std::endl;
  for (int i = 0; i < numWorkers_; ++i) {
    ret = insertWorker(i, workerCapacity_);
    if (!ret) {
      std::cout << "unable to add worker " << i << std::endl;
      return false;
    }
  }
  return true;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::teardown() {
  // Clean up data from previous run.
  truncateWorkerTable();
  truncateTaskTable();
  return true;
}

DbosStatus SinglePartitionedFIFOTaskScheduler::schedule() {
  //std::cout << "schedulerId is " << schedulerId_ << std::endl;
  int taskId = taskindex.fetch_add(1);
  DbosStatus status = probabilisticSelectTaskWorker(taskId);
  while (!status) {
    status = assignFromTaskQueue();
  }
  return true;
}