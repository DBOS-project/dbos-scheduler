#ifndef PARTITIONED_FIFO_TASK_SCHEDULER_H
#define PARTITIONED_FIFO_TASK_SCHEDULER_H

#include <atomic>
#include <string>
#include <vector>

#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct PartitionedFIFOScheduler.
class PartitionedFIFOTaskScheduler : public VoltdbSchedulerUtil {
public:
  PartitionedFIFOTaskScheduler(voltdb::Client* client, std::string dbAddr,
                               int partitions, int numTasks, int workerCapacity,
                               int numWorkers, float probMultiTx)
      : VoltdbSchedulerUtil(client, dbAddr),
        partitions_(partitions),
        numTasks_(numTasks),
        workerCapacity_(workerCapacity),
        numWorkers_(numWorkers),
        probMultiTx_(probMultiTx){};

  // Truncate the worker table;
  void truncateWorkerTable();

  // Truncate the worker table;
  void truncateTaskTable();

  // Insert a worker into the worker table.
  DbosStatus insertWorker(DbosId workerID, DbosId capacity);

  // Insert a task into the task table.
  DbosStatus insertTask(DbosId taskID);

  // Select a worker for a task and update worker capacity and task workerid.
  DbosStatus selectTaskWorker();

  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

  // Perform a scheduling act.
  DbosStatus schedule();

  // Destructor
  ~PartitionedFIFOTaskScheduler() { /* placeholder for now. */
  }

private:
  int partitions_;
  int numTasks_;
  int workerCapacity_;
  int numWorkers_;
  float probMultiTx_;
};

#endif  // #ifndef PARTITIONED_FIFO_TASK_SCHEDULER_H
