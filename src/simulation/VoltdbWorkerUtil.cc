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

#include "simulation/VoltdbWorkerUtil.h"

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

std::atomic<uint64_t> VoltdbWorkerUtil::totalTasks_;
std::atomic<uint64_t> VoltdbWorkerUtil::totalFinishedTasks_;

VoltdbWorkerUtil::~VoltdbWorkerUtil() {
  // placeholder.
}

voltdb::Client VoltdbWorkerUtil::createVoltdbClient(std::string dbAddr) {
  // Create a VoltDB client, connect to the DB.
  // SHA-256 can be used as of VoltDB5.2 by specifying voltdb::HASH_SHA256
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  try {
    client.createConnection(dbAddr);
  } catch (std::exception& e) {
    std::cerr << "An exception occured while connecting to VoltDB "
              << std::endl;
    // TODO: more robust error handling.
    throw;
  }
  return client;
}

VoltdbWorkerUtil::VoltdbWorkerUtil(DbosId workerId, std::string dbAddr)
    : workerId_(workerId), dbAddr_(dbAddr), stop_(false) {}
