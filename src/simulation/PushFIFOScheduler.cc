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

#include "simulation/PushFIFOScheduler.h"

#define SUCCESS 0
#define NOWORKER -2

static std::atomic<uint32_t> taskindex;

void PushFIFOScheduler::truncateTaskTable() {
  std::vector<voltdb::Parameter> parameterTypes(0);
  voltdb::Procedure procedure("TruncateTaskTable", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "TruncateTaskTable procedure failed. " << r.toString();
  }
}

DbosStatus PushFIFOScheduler::selectTaskWorker(DbosId taskID) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  // Actual num partitions.
  // TODO: fix this bug. need to use actual number of partitions.
  int pkey = rand() % partitions_;

  // Try to find an available worker in the partition.
  for (int count = 1; count <= partitions_; ++count) {
    voltdb::Procedure procedure("PushTask", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(pkey).addInt32(taskID);
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "PushFIFOTask procedure failed. "
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
    pkey = (pkey + count) % partitions_;
  }
  std::cout << "went through " << partitions_ << " partitions"
            << std::endl;
  return false;
}

DbosStatus PushFIFOScheduler::setup() {
  // Clean up data from previous run.
  truncateTaskTable();
  return true;
}

DbosStatus PushFIFOScheduler::teardown() {
  // Clean up data from previous run.
  truncateTaskTable();
  return true;
}

DbosStatus PushFIFOScheduler::schedule() {
  //for (int i = 0; i < numTasks_; ++i) {
  int taskId = taskindex.fetch_add(1);
  DbosStatus status = selectTaskWorker(taskId);
  assert(status == true);
  //}
  return true;
}
