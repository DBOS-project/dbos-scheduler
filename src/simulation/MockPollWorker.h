#ifndef MOCK_POLL_WORKER_H
#define MOCK_POLL_WORKER_H

#include <atomic>
#include <string>
#include <vector>

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
  void execute(); 

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
};

#endif  // #ifndef MOCK_POLL_WORKER_H
