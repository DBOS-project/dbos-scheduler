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

#include "simulation/VoltdbSchedulerUtil.h"

voltdb::Client VoltdbSchedulerUtil::createVoltdbClient(std::string username,
                                                       std::string password) {
  // Create a VoltDB client, connect to the DB.
  // SHA-256 can be used as of VoltDB5.2 by specifying voltdb::HASH_SHA256
  voltdb::ClientConfig config(username, password, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  return client;
}

VoltdbSchedulerUtil::VoltdbSchedulerUtil(voltdb::Client* client,
             	                         std::string dbAddr)
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
