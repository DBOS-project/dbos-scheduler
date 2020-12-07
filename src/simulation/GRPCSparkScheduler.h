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
  GRPCSparkScheduler(int port, std::string dbAddr,
                     int workerPartitions, int workerCapacity, int numWorkers)
      : port_(port),
        dbAddr(dbAddr),
        workerPartitions(workerPartitions),
        workerCapacity(workerCapacity),
        numWorkers(numWorkers){
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
  std::string dbAddr;
  int workerPartitions;
  int workerCapacity;
  int numWorkers;
  int port_;
  std::thread* workerThread_;
  std::unique_ptr<Server> workerServer_ = NULL;

};

#endif  // DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H
