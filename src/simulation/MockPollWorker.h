#ifndef MOCK_POLL_WORKER_H
#define MOCK_POLL_WORKER_H

#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "simulation/VoltdbWorkerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

class MockPollWorker : public VoltdbWorkerUtil {
public:
  MockPollWorker(int workerId, int pkey, std::string dbAddr,
                 int numExecutors)
      : VoltdbWorkerUtil(workerId, dbAddr),
        pkey_(pkey),
        numExecutors_(numExecutors) {};

  // Dispatch tasks that are assigned to this worker.
  // Potentially run in a dedicated dispatch thread.
  void dispatch();

  // Execute tasks. Potentially run in dedicated executor thread pool.
  // Borrow ideas from: https://github.com/embeddedartistry/embedded-resources/blob/master/examples/cpp/dispatch.cpp
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
  std::vector<std::thread*> threads_;  // including dispatch and executor threads
  std::queue<DbosId> taskQueue_;  // queue of task Ids
  std::condition_variable cv_;  // used to sync dispatch queue and executors
  bool stopDispatch_ = false;
};

#endif  // #ifndef MOCK_POLL_WORKER_H
