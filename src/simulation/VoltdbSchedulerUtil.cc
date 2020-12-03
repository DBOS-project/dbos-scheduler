// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/VoltdbSchedulerUtil.h"

VoltdbSchedulerUtil::~VoltdbSchedulerUtil() {
  // placeholder.
}

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
                                         std::string& dbAddr)
    : client_(client) {
  // Comma-separated list of hostnames or IPs.
  std::istringstream addrStream(dbAddr);
  std::string host;
  char delim = ',';

  // Method 1: Connect to each host, rely on VoltDB query affinity to route to
  // the correct host.
  // TODO: this is not the best practice. Each thread should connect to a
  // subset of hosts.

  while (std::getline(addrStream, host, delim)) {
    try {
      client_->createConnection(host);
    } catch (std::exception& e) {
      std::cerr << "An exception occured while connecting to VoltDB " << host
                << std::endl;
      std::cerr << e.what();
      // TODO: more robust error handling.
      throw;
    }
    std::cout << "=== Connected to VoltDB at " << host << " ===\n";
  }

  
  // Method 2: Randomly pick one host from the list; this may not be optimal as
  // well. We might need some hashing function or a way to figure out locality.
  /*
  std::vector<std::string> hostlist;
  while (std::getline(addrStream, host, delim)) {
    hostlist.push_back(host);
  }
  int randIndex = rand() % hostlist.size();
  host = hostlist[randIndex];
  try {
    client_->createConnection(host);
  } catch (std::exception& e) {
    std::cerr << "An exception occured while connecting to VoltDB " << host
              << std::endl;
    std::cerr << e.what();
    // TODO: more robust error handling.
    throw;
  }
  std::cout << "=== Connected to VoltDB at " << host << " ===\n";
  */

}
