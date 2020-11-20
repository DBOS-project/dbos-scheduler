// This file contains common utils for connecting with VoltDB
#ifndef DBOS_VOLTDB_SCHEDULER_UTIL_H
#define DBOS_VOLTDB_SCHEDULER_UTIL_H

#include <atomic>
#include <string>
#include <vector>

#include "voltdb-client-cpp/include/Client.h"

// Used for task_id, worker_id in DBOS.
// TODO: decide whether to use INT or STRING.
typedef int32_t DbosId;

// Used for status: true = succeeded, false = failed.
typedef bool DbosStatus;

// Note: this class is not thread safe, because voltDB client is not thread
// safe. Use one instance per thread.
// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct VoltdbSchedulerUtil.
class VoltdbSchedulerUtil {
public:
  VoltdbSchedulerUtil(voltdb::Client* client, std::string& dbAddr);

  // Perform a scheduling act.
  virtual DbosStatus schedule() = 0;

  // Setup the database to benchmark.
  virtual DbosStatus setup() = 0;

  // Tear down the database after benchmarking.
  virtual DbosStatus teardown() = 0;

  // Virutal destructor so that derived classes can be freed.
  virtual ~VoltdbSchedulerUtil() = 0;

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string username,
                                           std::string password);

protected:
  voltdb::Client* client_;
};

#endif  // #ifndef DBOS_VOLTDB_SCHEDULER_UTIL_H
