// Test functionality of PartitionedScanTask.

#include <string>
#include <vector>

#include "simulation/PartitionedScanTask.h"
#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient("testuser", "testpwd");
  PartitionedScanTask scheduler(&voltdbClient, "localhost", 8, 2, 2, 0.0);

  // Insert then Select a worker.
  DbosStatus ret = scheduler.setup();
  DbosId workerId;
  workerId = scheduler.selectMostTaskWorker();
  std::cout << "Selected: " << workerId << std::endl;
  return 0;
}
