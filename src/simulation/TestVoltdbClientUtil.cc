// Test functionality of VoltdbClientUtil.

#include <string>
#include <vector>

#include "VoltdbClientUtil.h"
#include "voltdb-client-cpp/include/Client.h"

int main(int argc, char** argv) {
  voltdb::Client voltdbClient =
      VoltdbClientUtil::createVoltdbClient("fakeuser", "fakepwd");
  VoltdbClientUtil client(&voltdbClient, "localhost");

  // Insert then Select a worker.
  DbosStatus ret = client.insertWorker(2, 2);
  auto workerId = client.selectWorker();
  std::cout << "Selected: " << workerId << std::endl;

  // Assign the task to worker.
  DbosId taskId(1);
  ret = client.assignTaskToWorker(taskId, workerId);

  return 0;
}
