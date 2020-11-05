// This file contains common utils for connecting with VoltDB
#ifndef DBOS_VOLTDB_WORKER_UTIL_H
#define DBOS_VOLTDB_WORKER_UTIL_H

#include <atomic>
#include <string>
#include <vector>

#include "voltdb-client-cpp/include/Client.h"

// Used for task_id, worker_id in DBOS.
// TODO: decide whether to use INT or STRING.
typedef int32_t DbosId;

// Used for status: true = succeeded, false = failed.
typedef bool DbosStatus;

// Note: Because voltDB client is not thread safe, use one VoltDB client
// instance per thread.
// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// Call static function createVoltdbClient() to get a local VoltDB client.
class VoltdbWorkerUtil {
public:
  // Task state.
  enum TaskState { UNKNOWN = 0, PENDING, RUNNING, COMPLETE };
  VoltdbWorkerUtil(DbosId workerId, std::string dbAddr);

  // Setup the worker.
  // E.g., setup dispatch thread, and multiple executor threads.
  virtual DbosStatus setup() = 0;

  // Stop the worker and free up resources.
  virtual DbosStatus teardown() = 0;

  // Virutal destructor so that derived classes can be freed.
  virtual ~VoltdbWorkerUtil() = 0;

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string dbAddr);

protected:
  std::string dbAddr_;  // Address to VoltDB server.
  DbosId workerId_;     // DBOS worker id.
  bool stop_;           // If true, stop all threads.
};

#endif  // #ifndef DBOS_VOLTDB_WORKER_UTIL_H
