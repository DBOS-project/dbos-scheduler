#ifndef MOCK_POLL_WORKER_H
#define MOCK_POLL_WORKER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "VoltdbWorkerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

class MockPollWorker : public VoltdbWorkerUtil {
public:
  MockPollWorker(int workerId, int pkey, std::string dbAddr, int numExecutors,
                 int topk)
      : VoltdbWorkerUtil(workerId, dbAddr),
        pkey_(pkey),
        numExecutors_(numExecutors),
        topk_(topk) {};

  // Dispatch tasks that are assigned to this worker.
  // Potentially run in a dedicated dispatch thread.
  void dispatch();

  // Execute tasks. Potentially run in dedicated executor thread pool.
  // Borrow ideas from:
  // https://github.com/embeddedartistry/embedded-resources/blob/master/examples/cpp/dispatch.cpp
  void execute(int execId);

  // Setup the worker.
  // E.g., setup dispatch thread, and multiple executor threads.
  DbosStatus setup();

  // Stop the worker (all executor and dispatch threads) and free up resources.
  DbosStatus teardown();

  // Destructor
  ~MockPollWorker() { /* placeholder for now. */
  }

private:
  int pkey_;
  int numExecutors_;
  std::mutex lock_;  // protect the access to shared taskQueue_
  std::vector<std::thread*>
      threads_;                   // including dispatch and executor threads
  std::queue<DbosId> taskQueue_;  // queue of task Ids
  std::condition_variable cv_;    // used to sync dispatch queue and executors
  bool stopDispatch_ = false;
  int topk_;  // Select top-K tasks in a batch
};

#endif  // #ifndef MOCK_POLL_WORKER_H
