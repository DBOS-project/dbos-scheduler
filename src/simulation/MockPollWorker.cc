// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/MockPollWorker.h"

DbosStatus MockPollWorker::setup() {
  std::cout << "Setup worker " << workerId_ << std::endl;

  // Start executors first, waiting for tasks.
  for (int i = 0; i < numExecutors_; ++i) {
    threads_.push_back(new std::thread(&MockPollWorker::execute, this));
  }

  // Start dispatch thread
  threads_.push_back(new std::thread(&MockPollWorker::dispatch, this));
  return true;
}

DbosStatus MockPollWorker::teardown() {
  // Clean up data and threads.
  std::cout << "Stop worker " << workerId_ << std::endl;
  stopDispatch_ = true;
  // Wait until the queue is empty
  while (taskQueue_.size()) {
    std::cout << "Waiting to drain task queue\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  // Stop all executors.
  std::unique_lock<std::mutex> lock(lock_);
  stop_ = true;
  lock.unlock();
  cv_.notify_all();

  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_[i]->join();
    delete threads_[i];
  }
  return true;
}

void MockPollWorker::dispatch() {
  //TODO
  std::cout << "A dispatcher for worker " << workerId_ << "\n";
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbWorkerUtil::createVoltdbClient();
  try {
    voltdbClient.createConnection(dbAddr_);
  } catch (std::exception& e) {
    std::cerr << "An exception occured while connecting to VoltDB "
              << std::endl;
    // TODO: more robust error handling.
    throw;
  }

  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  do {
    DbosId taskId = -1;
    // Select a task from DB.
	  voltdb::Procedure procedure("WorkerSelectTask",
	                              parameterTypes);
	  voltdb::ParameterSet* params = procedure.params();
	  params->addInt32(pkey_).addInt32(workerId_).addInt32(1);
	  voltdb::InvocationResponse r = voltdbClient.invoke(procedure);
	  if (r.failure()) {
	    std::cout << "WorkerSelecTask procedure failed. "
	              << r.toString() << std::endl;
	    continue;
	  }
	  std::vector<voltdb::Table> results = r.results();
    if (results.size() < 1) {
      std::cout << "Failed to find a task for worker " << workerId_ <<".\n";
      taskId = -1;
    } else {
      // TODO: support top-k.
      voltdb::Row row = results[0].iterator().next();
      taskId = row.getInt64(0);
	  }

    if (taskId >= 0) {
      std::unique_lock<std::mutex> lock(lock_);
      taskQueue_.push(taskId);
      // Manual unlocking before notifying.
      lock.unlock();
      cv_.notify_one();
      std::cout << "dispatch taskId " << taskId << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  } while (!stopDispatch_);
  std::cout << "Stopped dispatcher for worker " << workerId_ << "\n";
}

void MockPollWorker::execute() {
  // TODO
  std::cout << "An executor for worker " << workerId_ << "\n";
  std::unique_lock<std::mutex> lock(lock_);
  // Wait for tasks
  do {
    // Wait until we have a task, or stop signal.
    cv_.wait(lock, [this]{return (taskQueue_.size() || stop_); });

    // Get one task
    if (!stop_ && taskQueue_.size()) {
      DbosId taskId = taskQueue_.front();
      taskQueue_.pop();

      // unlock the lock.
      lock.unlock();

      std::cout << "worker " << workerId_ << " process taskId " << taskId << std::endl;

      // Re-acquire the lock.
      lock.lock();
    }
  } while (!stop_);
  std::cout << "Stopped executor for worker " << workerId_ << "\n";
}


