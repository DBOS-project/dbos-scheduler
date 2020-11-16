#ifndef MOCK_GRPC_WORKER_H
#define MOCK_GRPC_WORKER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "simulation/VoltdbWorkerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

class MockGRPCWorker : public VoltdbWorkerUtil {
public:
  MockGRPCWorker(voltdb::Client* voltdbClient, int workerId, int workerPartitions, int capacity, std::vector<int> workerData)
      : client_(voltdbClient),
	      VoltdbWorkerUtil(workerId, "bob"),
        workerData_(workerData),
        capacity_(capacity),
        workerPartitions_(workerPartitions) {};

  // Dispatch tasks that are assigned to this worker.
  // Potentially run in a dedicated dispatch thread.
  void dispatch();
  static void handle_request(struct http_request_s* request);

  // Execute tasks. Potentially run in dedicated executor thread pool.
  void execute(int execId);

  // Setup the worker.
  // E.g., setup dispatch thread, and multiple executor threads.
  DbosStatus setup();

  // Stop the worker (all executor and dispatch threads) and free up resources.
  DbosStatus teardown();

  // Destructor
  ~MockGRPCWorker() { /* placeholder for now. */
  }

private:
  voltdb::Client* client_;
  int workerPartitions_;
  int capacity_;
  std::vector<int> workerData_;
  std::vector<std::thread*>
      threads_;                   // including dispatch and executor threads
  bool stopDispatch_ = false;
};

#endif  // #ifndef MOCK_GRPC_WORKER_H
