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

#include "simulation/VoltdbClientUtil.h"

voltdb::Client VoltdbClientUtil::createVoltdbClient(std::string username,
                                                    std::string password) {
  // Create a VoltDB client, connect to the DB.
  // SHA-256 can be used as of VoltDB5.2 by specifying voltdb::HASH_SHA256
  voltdb::ClientConfig config(username, password, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  return client;
}

VoltdbClientUtil::VoltdbClientUtil(voltdb::Client* client, std::string dbAddr)
    : client_(client) {
  try {
    client_->createConnection(dbAddr);
  } catch (std::exception& e) {
    std::cerr << "An exception occured while connecting to VoltDB "
              << std::endl;
    // TODO: more robust error handling.
    throw;
  }
  std::cout << "=== Connected to VoltDB at " << dbAddr << " ===\n";
}

DbosStatus VoltdbClientUtil::insertWorker(DbosId workerID, DbosId capacity) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("InsertWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerID).addInt32(capacity);
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  } else {
    std::cout << "Insert Worker: " << workerID << std::endl;
  }
  return true;
}

DbosId VoltdbClientUtil::selectWorker() {
  // TODO: implement the actual transaction here.
  std::cout << "selectWorker, to be implemented." << std::endl;
  DbosId workerId(1);

  return workerId;
}

DbosStatus VoltdbClientUtil::assignTaskToWorker(DbosId taskId,
                                                DbosId workerId) {
  // TODO: implement the actual transaction here.
  std::cout << "assignTaskToWorker, to be implemented." << std::endl;
  std::cout << taskId << " -> " << workerId << std::endl;
  DbosStatus retStatus = true;

  return retStatus;
}
