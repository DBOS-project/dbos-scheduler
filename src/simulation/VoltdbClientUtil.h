// This file contains common utils for connecting with VoltDB
#ifndef DBOS_VOLTDB_CLIENT_UTIL_H
#define DBOS_VOLTDB_CLIENT_UTIL_H

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
// (2) pass the pointer to construct VoltdbClientUtil.
class VoltdbClientUtil {
public:
  VoltdbClientUtil(voltdb::Client* client, std::string dbAddr, int workerPartitions);

  // Truncate the worker talbe;
  void truncateWorkerTable();

  // Insert a worker into the worker table.
  DbosStatus insertWorker(DbosId workerID, DbosId capacity);

  // Select a worker for a task and update worker capacity.
  // Return the selected worker id.
  DbosId selectWorker();

  // Update which worker the task is assigned to, and update worker status to
  // scheduled.
  DbosStatus assignTaskToWorker(DbosId taskId, DbosId workerId);

  // Complete a task, updating the capacity of its worker.
  DbosStatus finishTask(DbosId taskId, DbosId workerId);

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string username,
                                           std::string password);

private:
  voltdb::Client* client_;
  int workerPartitions_;
  static std::atomic<int> numWorkers_;
};

#endif  // #ifndef DBOS_VOLTDB_CLIENT_UTIL_H
