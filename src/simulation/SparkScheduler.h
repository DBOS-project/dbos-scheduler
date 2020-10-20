// This file contains common utils for connecting with VoltDB
#ifndef SPARK_SCHEDULER_H
#define SPARK_SCHEDULER_H

#include <atomic>
#include <string>
#include <vector>

#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct SparkScheduler.
class SparkScheduler : public VoltdbSchedulerUtil {
public:
  SparkScheduler(voltdb::Client* client, std::string dbAddr,
                           int workerPartitions, int workerCapacity,
                           int numWorkers)
      : VoltdbSchedulerUtil(client, dbAddr),
        workerPartitions_(workerPartitions),
        workerCapacity_(workerCapacity),
        numWorkers_(numWorkers){};

  // Truncate the worker table;
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

  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

  // Perform a scheduling act.
  DbosStatus schedule();

  // Destructor
  ~SparkScheduler() { /* placeholder for now. */
  }

private:
  int workerCapacity_;
  int workerPartitions_;
  int numWorkers_;
};

#endif
