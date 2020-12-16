// Test functionality of SparkScheduler.

#include <string>
#include <vector>

#include "SparkScheduler.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      SparkScheduler::createVoltdbClient("testuser", "testpwd");
  // Client - Host - Partitions - Capacity - numWorkers
  SparkScheduler scheduler(&voltdbClient, "localhost", 1, 1, 2);

  // Insert then Select a worker.
  DbosStatus ret = scheduler.setup();
  assert(ret);
  DbosId workerId;

  Task* task = new Task;
  task->execTime = 1000;

  task->targetData = 1;
  scheduler.schedule(task);

  task->targetData = 0;
  scheduler.schedule(task);

  task->targetData = 1;
  scheduler.schedule(task);

  task->targetData = 0;
  scheduler.schedule(task);

  task->targetData = 0;
  scheduler.schedule(task);

  delete task;

  scheduler.teardown();
  return 0;
}
