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

#include "simulation/PartitionedScanTask.h"

#define SUCCESS 0
#define NOWORKER -2

void PartitionedScanTask::truncateTaskTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateTaskTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateTaskTable procedure failed. " << r.toString();
  }
}

DbosStatus PartitionedScanTask::insertTask(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  DbosId workerID = taskID % numWorkers_;  // assign a worker to the task.
  params->addInt32(taskID).addInt32(workerID).addInt32(1).addInt32(workerID %
                                                                   partitions_);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertTask procedure failed. " << r.toString();
    return false;
  }
  return true;
}

DbosId PartitionedScanTask::selectMostTaskWorker() {
  std::vector<voltdb::Parameter> parameterTypes(1);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  // Actual num partitions.
  int activePartitions =
      std::min(partitions_, std::min(numTasks_, numWorkers_));
  int pkey = rand() % activePartitions;

  // Throw a coin and decide whether to use single partition transaction.
  float coin = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

  if (coin > probMultiTx_) {
    // Try to find a worker with most tasks in a single partition.
    for (int count = 1; count <= activePartitions; ++count) {
      voltdb::Procedure procedure("ScanPartitionedTaskWorker", parameterTypes);
      voltdb::ParameterSet* params = procedure.params();
      params->addInt32(pkey);
      voltdb::InvocationResponse r = client_->invoke(procedure);
      if (r.failure()) {
        std::cout << "ScanPartitionedTaskWorker procedure failed. "
                  << r.toString() << std::endl;
        return false;
      }
      std::vector<voltdb::Table> results = r.results();
      voltdb::Row row = results[0].iterator().next();
      int workerId = row.getInt64(0);
      if (workerId >= 0) {
        return workerId;
      } else {
        // If there is no task in this partition, we need to change partition.
        pkey = (count + pkey) % activePartitions;
      }
    }
  }

  // Check all partitions.
  parameterTypes.clear();
  voltdb::Procedure naive_procedure("ScanTaskWorker", parameterTypes);
  voltdb::InvocationResponse r = client_->invoke(naive_procedure);
  if (r.failure()) {
    std::cout << "ScanTaskWorker procedure failed. " << r.toString()
              << std::endl;
    return false;
  }
  std::vector<voltdb::Table> results = r.results();
  voltdb::Row row = results[0].iterator().next();
  int workerId = row.getInt64(0);
  if (workerId >= 0) { return workerId; }

  return NOWORKER;
}

DbosStatus PartitionedScanTask::setup() {
  // Clean up data from previous run.
  truncateTaskTable();
  DbosStatus ret;
  std::cout << "Insert tasks" << std::endl;
  for (int i = 0; i < numTasks_; ++i) {
    ret = insertTask(i);
    if (!ret) { return false; }
  }
  return true;
}

DbosStatus PartitionedScanTask::teardown() {
  // Clean up data from previous run.
  // truncateTaskTable();
  return true;
}

DbosStatus PartitionedScanTask::schedule() {
  DbosId status = selectMostTaskWorker();
  assert(status >= 0);
  return true;
}
