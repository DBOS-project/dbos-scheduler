// This file contains common utils for connecting with VoltDB
#ifndef DBOS_VOLTDB_CLIENT_UTIL_H
#define DBOS_VOLTDB_CLIENT_UTIL_H

#include <string>
#include <vector>

#include "voltdb-client-cpp/include/Client.h"

// Used for task_id, worker_id in DBOS.
typedef std::string DbosId;

// Used for status: true = succeeded, false = failed.
typedef bool DbosStatus;

// Note: this class is not thread safe, because voltDB client is not thread
// safe. Use one instance per thread.
// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct VoltdbClientUtil.
class VoltdbClientUtil {
public:
  VoltdbClientUtil(voltdb::Client* client, std::string dbAddr);

  // Insert a worker into the worker table.
  void insertWorker(int workerID, int capacity);

  // Select a worker for a task and update worker capacity.
  // Return the selected worker id.
  DbosId selectWorker();

  // Update which worker the task is assigned to, and update worker status to
  // scheduled.
  DbosStatus assignTaskToWorker(DbosId taskId, DbosId workerId);

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string username,
                                           std::string password);

private:
  voltdb::Client* client_;
};

#endif  // #ifndef DBOS_VOLTDB_CLIENT_UTIL_H
