// This synthetic multi-thread scheduler will create "fake" task ids, and
// queries the DB to find a worker.  There won't be actual worker or load
// generator.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel scheudlers.

#include <string>
#include <vector>

#include "VoltdbClientUtil.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      VoltdbClientUtil::createVoltdbClient("fakeuser", "fakepwd");
  VoltdbClientUtil client(&voltdbClient, "localhost");

  // Select a worker.
  client.insertWorker(1, 1);
  auto workerId = client.selectWorker();
  std::cout << workerId << std::endl;

  // Assign the task to worker.
  DbosId taskId("testtaskid");
  auto ret = client.assignTaskToWorker(taskId, workerId);

  return 0;
}
