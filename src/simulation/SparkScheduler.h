// This file contains common utils for connecting with VoltDB
#ifndef SPARK_SCHEDULER_H
#define SPARK_SCHEDULER_H

#include <atomic>
#include <string>
#include <vector>

#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

#include "simulation/VoltdbWorkerUtil.h"
#include "simulation/MockGRPCWorker.h"

using grpc::CompletionQueue;
using grpc::ClientAsyncResponseReader;

// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct SparkScheduler.
class SparkScheduler : public VoltdbSchedulerUtil {
public:
  SparkScheduler(voltdb::Client* client, std::string dbAddr,
                 int workerPartitions, int workerCapacity, int numWorkers)
      : VoltdbSchedulerUtil(client, dbAddr),
        workerPartitions_(workerPartitions),
        workerCapacity_(workerCapacity),
        numWorkers_(numWorkers){};

  // Truncate the worker table;
  void truncateWorkerTable();

  // Insert a worker into the worker table.
  DbosStatus insertWorker(DbosId workerID, DbosId capacity, std::vector<int32_t> workerData);

  // Select a worker for a task and update worker capacity.
  // Return the selected worker id.
  DbosId selectWorker(DbosId targetData);

  // Update which worker the task is assigned to, and update worker status to
  // scheduled.
  DbosStatus assignTaskToWorker(DbosId taskId, DbosId workerId);

  // Complete a task, updating the capacity of its worker.
  DbosStatus finishTask(voltdb::Client client, DbosId taskId, DbosId workerId);

  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

  // Start a scheduler instance.
  DbosStatus startInstance();

  // Terminate a scheduler instance.
  DbosStatus terminateInstance();

  // Perform a scheduling act.
  DbosStatus schedule();

  // Enqueue a task.
  DbosStatus enqueue(int taskID, int targetData);

  // Destructor
  ~SparkScheduler() { /* placeholder for now. */
  }

private:

    struct AsyncClientCall {
        // Container for the data we expect from the server.
        dbos_scheduler::SubmitTaskResponse reply;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;
        // Storage for the status of the RPC upon completion.
        Status status;
        // ID of worker to which call was made.
        int workerID;
        std::unique_ptr<ClientAsyncResponseReader<dbos_scheduler::SubmitTaskResponse>> response_reader;
    };

  struct TaskData {
    // Task ID;
    DbosId taskID;
    // Target data for task.
    int targetData;
  };

  std::shared_ptr<Channel> addrToChannel(std::string workerAddr);
  std::unordered_map<std::string, std::shared_ptr<Channel>> channelMap;

  int workerCapacity_;
  int workerPartitions_;
  int numWorkers_;
  int dataPerWorker_ = 10;
  CompletionQueue cq_;
  std::atomic_int taskIDs;
  std::thread* finishRequestsThread_ = NULL;
  std::thread* processTaskQueueThread_ = NULL;
  bool runTaskQueueThread = true;

  static std::vector<VoltdbWorkerUtil*> workers_;

  void finishRequests();
  void processTaskQueue();

  std::queue<TaskData*> taskQueue;
};

#endif
