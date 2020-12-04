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
  GRPCSparkScheduler(int port, voltdb::Client* client, std::string dbAddr,
                     int workerPartitions, int workerCapacity, int numWorkers)
      : port_(port),
        client(client),
        dbAddr(dbAddr),
        workerPartitions(workerPartitions),
        workerCapacity(workerCapacity),
        numWorkers(numWorkers){};
  // Setup the database.
  DbosStatus setup();

  // Tear down the database after benchmarking.
  DbosStatus teardown();

private:
  void RunServer();
  voltdb::Client* client;
  std::string dbAddr;
  int workerPartitions;
  int workerCapacity;
  int numWorkers;
  int port_;
  std::thread* workerThread_;
  std::unique_ptr<Server> workerServer_ = NULL;

};

#endif  // DBOS_SCHEDULER_GRPCSPARKSCHEDULER_H
