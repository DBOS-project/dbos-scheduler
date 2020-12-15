// This file contains common utils for connecting with VoltDB
#ifndef DBOS_VOLTDB_SCHEDULER_UTIL_H
#define DBOS_VOLTDB_SCHEDULER_UTIL_H

#include <atomic>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ProcedureCallback.hpp"
#include "Task.h"

// Used for task_id, worker_id in DBOS.
// TODO: decide whether to use INT or STRING.
typedef int32_t DbosId;

// Used for status: true = succeeded, false = failed.
typedef bool DbosStatus;

// Note: this class is not thread safe, because voltDB client is not thread
// safe. Use one instance per thread.
// Since voltdb::Client only has private constructor, we cannot create a private
// member variable of it. Each thread needs to:
// (1) call static function createVoltdbClient() to get a local VoltDB client.
// (2) pass the pointer to construct VoltdbSchedulerUtil.
class VoltdbSchedulerUtil {
public:
  VoltdbSchedulerUtil(voltdb::Client* client, std::string& dbAddr);

  // Schedule and execute a task, return its status when complete.
  virtual DbosStatus schedule(Task* task) = 0;

  // Async schedule. Will return immediately without waiting for response.
  // TODO: turn this into a pure virtual function.
  virtual DbosStatus asyncSchedule(boost::shared_ptr<voltdb::ProcedureCallback> callback) {
    std::cerr << "Not implemented\n";
    abort();
  }

  // Send message to <num_partitions> DB partitions starting from <partition>.
  // TODO: use an array of partition numbers as argument.
  // TODO: include message content.
  virtual DbosStatus asyncSendMessage(int partition, int num_partitions,
      boost::shared_ptr<voltdb::ProcedureCallback> callback) {
    for (int i = partition; i < partition + num_partitions; ++i) {
      std::vector<voltdb::Parameter> parameterTypes(1);
      parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
      voltdb::Procedure procedure("SendMessage", parameterTypes);
      voltdb::ParameterSet* params = procedure.params();
      params->addInt32(i);
      client_->invoke(procedure, callback);
    }
    return true;
  }

  // Broadcast message to all DB partitions using a replicated table.
  // TODO: include message content.
  virtual DbosStatus asyncBroadcastMessage(boost::shared_ptr<voltdb::ProcedureCallback> callback) {
    std::vector<voltdb::Parameter> parameterTypes(1);
    parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
    voltdb::Procedure procedure("BroadcastMessage", parameterTypes);
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(0);
    client_->invoke(procedure, callback);
    return true;
  }

  // Setup the database to benchmark.
  virtual DbosStatus setup() = 0;

  // Tear down the database after benchmarking.
  virtual DbosStatus teardown() = 0;

  // Virutal destructor so that derived classes can be freed.
  virtual ~VoltdbSchedulerUtil() = 0;

  // Create a VoltDB client and return.
  static voltdb::Client createVoltdbClient(std::string username,
                                           std::string password);

protected:
  voltdb::Client* client_;
};

#endif  // #ifndef DBOS_VOLTDB_SCHEDULER_UTIL_H
