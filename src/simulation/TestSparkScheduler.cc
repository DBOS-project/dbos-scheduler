// Test functionality of SparkScheduler.

#include <string>
#include <vector>

#include "simulation/SparkScheduler.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      SparkScheduler::createVoltdbClient("testuser", "testpwd");
  SparkScheduler scheduler(&voltdbClient, "localhost", 8, 2, 1);

  // Insert then Select a worker.
  scheduler.truncateWorkerTable();
  DbosStatus ret = scheduler.setup();
  DbosId workerId;
  workerId = scheduler.selectWorker(0);
  std::cout << "Selected: " << workerId << std::endl;
  ret = scheduler.finishTask(voltdbClient, 0, workerId);
  workerId = scheduler.selectWorker(1);
  std::cout << "Selected: " << workerId << std::endl;
  ret = scheduler.finishTask(voltdbClient, 0, workerId);

  workerId = scheduler.selectWorker(0);
  scheduler.finishTask(voltdbClient, 0, workerId);
  workerId = scheduler.selectWorker(0);
  scheduler.finishTask(voltdbClient, 0, workerId);
  workerId = scheduler.selectWorker(0);
  scheduler.finishTask(voltdbClient, 0, workerId);

  scheduler.schedule();

  return 0;
}
