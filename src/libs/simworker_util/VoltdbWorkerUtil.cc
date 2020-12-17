// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "VoltdbWorkerUtil.h"

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

  // Comma-separated list of hostnames or IPs.
  std::istringstream addrStream(dbAddr);
  std::string host;
  char delim = ',';

  // TODO:for now every scheduler/worker would randomly connect to one host and
  // all queries will go through that host. We may develop some
  // locality/affinity, or hashing methods, to improve the locality.
  // Method 2: Randomly pick one host from the list; this may not be optimal as
  // well. We might need some hashing function or a way to figure out locality.
  std::vector<std::string> hostlist;
  while (std::getline(addrStream, host, delim)) { hostlist.push_back(host); }
  int randIndex = rand() % hostlist.size();
  host = hostlist[randIndex];
  try {
    client.createConnection(host);
  } catch (std::exception& e) {
    std::cerr << "An exception occured while connecting to VoltDB " << host
              << std::endl;
    std::cerr << e.what();
    // TODO: more robust error handling.
    throw;
  }

  return client;
}

VoltdbWorkerUtil::VoltdbWorkerUtil(DbosId workerId, std::string dbAddr)
    : workerId_(workerId), dbAddr_(dbAddr), stop_(false) {}
