// Test functionality of SparkScheduler.

#include <string>
#include <vector>

#include "simulation/SparkScheduler.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      SparkScheduler::createVoltdbClient("testuser", "testpwd");
  // Client - Host - Partitions - Capacity - numWorkers
  SparkScheduler scheduler(&voltdbClient, "localhost", 1, 1, 2);

  // Insert then Select a worker.
  scheduler.truncateWorkerTable();
  DbosStatus ret = scheduler.setup();
  assert(ret);
  DbosId workerId;

  workerId = scheduler.selectWorker(0);
  assert(workerId != -1);
  ret = scheduler.finishTask(voltdbClient, 0, workerId);

  workerId = scheduler.selectWorker(1);
  assert(workerId != -1);
  ret = scheduler.finishTask(voltdbClient, 0, workerId);

  workerId = scheduler.selectWorker(0);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(0);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(0);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(1);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(0);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(1);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  workerId = scheduler.selectWorker(1);
  assert(workerId != -1);
  scheduler.finishTask(voltdbClient, -1, workerId);

  Task* task = new Task;
  task->execTime = 1000;
  task->requirement = 1;
  scheduler.schedule(task);
  delete task;

  scheduler.teardown();
  return 0;
}
