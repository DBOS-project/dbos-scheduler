#ifndef PUSH_FIFO_SCHEDULER_H
#define PUSH_FIFO_SCHEDULER_H

#include <atomic>
#include <string>
#include <vector>

#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct PartitionedFIFOScheduler.
class PushFIFOScheduler : public VoltdbSchedulerUtil {
public:
  PushFIFOScheduler(voltdb::Client* client, std::string dbAddr,
                        int partitions, int numTasks, float probMultiTx)
      : VoltdbSchedulerUtil(client, dbAddr),
        partitions_(partitions),
        numTasks_(numTasks),
        probMultiTx_(probMultiTx){};

  // Truncate the worker table;
  void truncateWorkerTable();

  // Truncate the worker table;
  void truncateTaskTable();

  // Select a worker for a task and update worker capacity and task workerid.
  DbosStatus selectTaskWorker(DbosId taskID);

  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

  // Perform a scheduling act.
  DbosStatus schedule();

  // Destructor
  ~PushFIFOScheduler() { /* placeholder for now. */
  }

private:
  int partitions_;
  int numTasks_;
  float probMultiTx_;
};

#endif  // #ifndef PUSH_FIFO_SCHEDULER_H
