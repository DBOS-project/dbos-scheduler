// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include <thread>
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
#define NOWORKER -2

static const int MAXTRY = 1000;  // At most try 1000 times before returning false.
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

DbosStatus SinglePartitionedFIFOTaskScheduler::selectTaskWorker(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  // Actual num partitions.
  int activePartitions = std::min(partitions_, numWorkers_);
  int pkey = rand() % activePartitions;

  // Try multiple times before giving up.
  for (int t = 0; t < MAXTRY; t++) {
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
      } else if (status == NOWORKER) {  // implementing basic functionality first
        // std::cout << "no worker in partition " << count << std::endl;
        // TODO: If there is no worker, need to check other partitions for
        // available workers.
        // TODO: If there are no available workers, put task in a buffer
      }
      pkey = (pkey + count) % activePartitions;
    }
    // TODO: Sleep 100us is a naive choice right now, to avoid overloading DB.
    // Because if it cannot find a worker in all partitions, it will need to
    // wait for the worker to release one capacity and it may take about 100us.
    // We can refactor this code once we have a task buffer.
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
  std::cout << "went through " << activePartitions << " partitions"
            << std::endl;
  return false;
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

DbosStatus SinglePartitionedFIFOTaskScheduler::schedule(Task* task) {
  int taskId = taskindex.fetch_add(1);
  DbosStatus status = selectTaskWorker(taskId);
  assert(status == true);
  return true;
}
