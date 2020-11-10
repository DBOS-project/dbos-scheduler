#ifndef MOCK_HTTP_WORKER_H
#define MOCK_HTTP_WORKER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "httpserver.h"
#include "simulation/VoltdbWorkerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

class MockHTTPWorker : public VoltdbWorkerUtil {
public:
  MockHTTPWorker(voltdb::Client* voltdbClient, int workerId, int pkey, std::string dbAddr)
      : client_(voltdbClient),
	VoltdbWorkerUtil(workerId, dbAddr),
        pkey_(pkey) {};

  // Dispatch tasks that are assigned to this worker.
  // Potentially run in a dedicated dispatch thread.
  void dispatch();
  static void handle_request(struct http_request_s* request);

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
  ~MockHTTPWorker() { /* placeholder for now. */
  }

private:
  voltdb::Client* client_;
  int pkey_;
  std::vector<std::thread*>
      threads_;                   // including dispatch and executor threads
  bool stopDispatch_ = false;
};

#endif  // #ifndef MOCK_HTTP_WORKER_H
