#ifndef PARTITIONED_SCAN_TASK_H
#define PARTITIONED_SCAN_TASK_H

#include <atomic>
#include <string>
#include <vector>

#include "VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct PartitionedScanTask.
class PartitionedScanTask : public VoltdbSchedulerUtil {
public:
  PartitionedScanTask(voltdb::Client* client, std::string dbAddr,
                      int partitions, int numTasks, int numWorkers,
                      float probMultiTx)
      : VoltdbSchedulerUtil(client, dbAddr),
        partitions_(partitions),
        numTasks_(numTasks),
        numWorkers_(numWorkers),
        probMultiTx_(probMultiTx){};

  // Truncate the task table;
  void truncateTaskTable();

  // Insert a task into the task table.
  DbosStatus insertTask(DbosId taskID);

  // Select a worker with most tasks.
  DbosId selectMostTaskWorker();

  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

  // Perform a scheduling act.
  DbosStatus schedule(Task* task);

  // Destructor
  ~PartitionedScanTask() { /* placeholder for now. */
  }

private:
  int partitions_;
  int numTasks_;
  int numWorkers_;
  float probMultiTx_;
};

#endif  // #ifndef PARTITIONED_SCAN_TASK_H
