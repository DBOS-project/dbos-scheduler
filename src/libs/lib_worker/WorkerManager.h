// This file contains common utils for connecting with VoltDB
#ifndef DBOS_WORKER_MANAGER_H
#define DBOS_WORKER_MANAGER_H

#include <atomic>
#include <string>
#include <vector>

#include "DbosDefs.h"
#include "voltdb-client-cpp/include/Client.h"

// Note: Because voltDB client is not thread safe, use one VoltDB client
// instance per thread.
// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// Call static function createVoltdbClient() to get a local VoltDB client.
class WorkerManager {
public:
  // Task state.
  enum TaskState { UNKNOWN = 0, PENDING, RUNNING, COMPLETE };
  WorkerManager(DbosId workerId, std::string dbAddr);

  // Setup the worker.
  // E.g., setup dispatch thread, and multiple executor threads.
  virtual DbosStatus startServing() = 0;

  // Stop the worker and free up resources.
  virtual DbosStatus endServing() = 0;

  // Virutal destructor so that derived classes can be freed.
  virtual ~WorkerManager() = 0;

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string dbAddr);

  static std::atomic<uint64_t> totalTasks_;
  static std::atomic<uint64_t> totalFinishedTasks_;
  std::string workerAddr;

protected:
  std::string dbAddr_;  // Address to VoltDB server.
  DbosId workerId_;     // DBOS worker id.
  bool stop_;           // If true, stop all threads.
};

#endif  // #ifndef DBOS_WORKER_MANAGER_H
