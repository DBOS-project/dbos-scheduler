// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include <thread>
#include <functional>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/MockGRPCWorker.h"

DbosStatus MockGRPCWorker::setup() {
  // Add the Worker to the database.
  std::vector<voltdb::Parameter> parameterTypes(5);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[4] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);

  voltdb::Procedure procedure("InsertSparkWorker", parameterTypes);
  for (int data: workerData_) {
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(workerId_)
        .addInt32(capacity_)
        .addInt32(data)
        .addInt32(workerId_ % workerPartitions_)
        .addString("");
    voltdb::InvocationResponse r = client_->invoke(procedure);
    if (r.failure()) {
      std::cout << "InsertWorker procedure failed. " << r.toString();
      return false;
    }
  }

  return true;
}

DbosStatus MockGRPCWorker::teardown() {
  // Clean up data and threads.
  std::cout << "Stop worker " << workerId_ << std::endl;
  return true;
}
