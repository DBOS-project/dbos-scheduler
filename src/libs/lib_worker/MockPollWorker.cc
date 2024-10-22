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

#include "MockExecutor.h"
#include "MockPollWorker.h"

DbosStatus MockPollWorker::startServing() {
  std::cout << "Setup worker " << workerId_ << std::endl;

  // Start executors first, waiting for tasks.
  for (int i = 0; i < numExecutors_; ++i) {
    threads_.push_back(new std::thread(&MockPollWorker::execute, this, i));
  }

  // Start dispatch thread
  threads_.push_back(new std::thread(&MockPollWorker::dispatch, this));
  return true;
}

DbosStatus MockPollWorker::endServing() {
  // Clean up data and threads.
  std::cout << "Stop worker " << workerId_ << std::endl;
  stopDispatch_ = true;

  // Join dispatch thread first.
  size_t totalThreads = threads_.size();
  threads_[totalThreads - 1]->join();
  delete threads_[totalThreads - 1];

  // Wait until the queue is empty
  while (taskQueue_.size()) {
    std::cout << "Waiting to drain task queue\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  // Stop all executors.
  {
    std::lock_guard<std::mutex> lock(lock_);
    stop_ = true;
  }
  cv_.notify_all();

  for (size_t i = 0; i < totalThreads - 1; ++i) {
    threads_[i]->join();
    delete threads_[i];
  }
  return true;
}

void MockPollWorker::dispatch() {
  std::cout << "A dispatcher for worker " << workerId_ << "\n";
  // Create a local VoltDB client.
  voltdb::Client voltdbClient = WorkerManager::createVoltdbClient(dbAddr_);

  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  voltdb::Procedure procedure("WorkerSelectTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  do {
    DbosId taskId = -1;
    // Select top-k task(s) from DB.
    params->addInt32(pkey_).addInt32(workerId_).addInt32(topk_);

    voltdb::InvocationResponse r = voltdbClient.invoke(procedure);
    if (r.failure()) {
      std::cout << "WorkerSelecTask procedure failed. " << r.toString()
                << std::endl;
      continue;
    }
    voltdb::Table results = r.results()[0];
    voltdb::TableIterator resIter = results.iterator();
    // std::cout << r.toString();
    while (resIter.hasNext()) {
      voltdb::Row row = resIter.next();
      taskId = row.getInt32(0);

      {
        std::lock_guard<std::mutex> lock(lock_);
        taskQueue_.push(taskId);
      }
      cv_.notify_one();
      WorkerManager::totalTasks_.fetch_add(1);
      // std::cout << "dispatch taskId " << taskId << "\n";
    }
    // Busy polling or sleep?
    // std::this_thread::sleep_for(std::chrono::microseconds(10 * topk_));
  } while (!stopDispatch_);
  std::cout << "Stopped dispatcher for worker " << workerId_ << "\n";
}

void MockPollWorker::execute(int execId) {
  std::cout << "Executor " << execId << " for worker " << workerId_ << "\n";
  MockExecutor executor = MockExecutor();

  // Create a local VoltDB client.
  voltdb::Client voltdbClient = WorkerManager::createVoltdbClient(dbAddr_);

  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  voltdb::Procedure procedure("WorkerUpdateTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();

  std::unique_lock<std::mutex> lock(lock_);

  // Wait for tasks
  do {
    // Wait until we have a task, or stop signal.
    cv_.wait(lock, [this] { return (taskQueue_.size() || stop_); });

    // Get one task
    if (!stop_ && taskQueue_.size()) {
      DbosId taskId = taskQueue_.front();
      taskQueue_.pop();

      // unlock the lock.
      lock.unlock();

      // std::cout << "worker " << workerId_ << " executor " << execId
      //          << " process taskId " << taskId << std::endl;

      // TODO: add parameter to mock execution time.
      // Maybe move this whole function to the Executor class.
      executor.executeTask();

      // std::this_thread::sleep_for(std::chrono::microseconds(100));
      // Update task as completed from DB, and put back one capacity.
      params->addInt32(pkey_).addInt32(workerId_).addInt32(taskId).addInt32(
          COMPLETE);

      voltdb::InvocationResponse r = voltdbClient.invoke(procedure);
      if (r.failure()) {
        std::cout << "WorkerUpdateTask procedure failed. " << r.toString()
                  << std::endl;
        abort();  // TODO: better error handling.
      }

      WorkerManager::totalFinishedTasks_.fetch_add(1);
      // Re-acquire the lock.
      lock.lock();
    }
  } while (!stop_);
  std::cout << "Stopped executor " << execId << " for worker " << workerId_
            << "\n";
}
