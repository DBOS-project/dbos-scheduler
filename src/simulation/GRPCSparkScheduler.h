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
  GRPCSparkScheduler(int port, VoltdbSchedulerUtil* scheduler)
      : port_(port),
        scheduler_(scheduler) {
          workerThread_ = new std::thread(&GRPCSparkScheduler::RunServer, this);
          while (workerServer_ == NULL) {;} // Spin until server is online.
        };

  ~GRPCSparkScheduler() {
    // Clean up data and threads.
    workerServer_->Shutdown();
    workerThread_->join();
    delete workerThread_;
  }

private:
  void RunServer();
  int port_;
  VoltdbSchedulerUtil* scheduler_;
  std::thread* workerThread_;
  std::unique_ptr<Server> workerServer_ = NULL;

};

#endif  // DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H
