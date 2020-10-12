// Test functionality of VoltdbClientUtil.

#include <string>
#include <vector>

#include "simulation/PartitionedFIFOScheduler.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      PartitionedFIFOScheduler::createVoltdbClient("testuser", "testpwd");
  PartitionedFIFOScheduler scheduler(&voltdbClient, "localhost", 8, 2, 2);

  // Insert then Select a worker.
  scheduler.truncateWorkerTable();
  DbosStatus ret = scheduler.setup();
  auto workerId = scheduler.selectWorker();
  std::cout << "Selected: " << workerId << std::endl;

  // Assign the task to worker.
  DbosId taskId(1);
  ret = scheduler.assignTaskToWorker(taskId, workerId);

  ret = scheduler.finishTask(taskId, workerId);

  return 0;
}
