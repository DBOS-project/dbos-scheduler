#ifndef DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H
#define DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H

#include <atomic>
#include <string>
#include <vector>

#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

#include "simulation/VoltdbWorkerUtil.h"
#include "simulation/MockGRPCWorker.h"

class GRPCSparkScheduler {
public:
  GRPCSparkScheduler(int port)
      : port_(port){};
  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

private:
  void RunServer();
  int port_;
  std::thread* workerThread_;
  std::unique_ptr<Server> workerServer_ = NULL;

};

#endif  // DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H
