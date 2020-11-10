// This file contains common utils for connecting with VoltDB
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <vector>
#include <thread>
#include <functional>
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

#include "simulation/MockHTTPWorker.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"

thread_local MockHTTPWorker* worker;

DbosStatus MockHTTPWorker::setup() {
  std::cout << "Setup worker " << workerId_ << std::endl;

  // Add the Worker to the database.
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);

  voltdb::Procedure procedure("InsertWorker", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(workerId_).addInt32(999999999/*capacity*/).addInt32(pkey_)
	  .addString("http://localhost:" + std::to_string(9090 + workerId_));
  voltdb::InvocationResponse r = client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "InsertWorker procedure failed. " << r.toString();
    return false;
  }

  // Start dispatch thread.
  threads_.push_back(new std::thread(&MockHTTPWorker::dispatch, this));
  return true;
}

DbosStatus MockHTTPWorker::teardown() {
  // Clean up data and threads.
  std::cout << "Stop worker " << workerId_ << std::endl;
  stopDispatch_ = true;
  return true;
}

#define RESPONSE "Hello, World!"
void MockHTTPWorker::handle_request(struct http_request_s* request) {
  struct http_string_s taskIdRaw = http_request_header(request, "taskID");
  std::string taskIdStr(taskIdRaw.buf, taskIdRaw.len);
  int taskId = std::stoi(taskIdStr);

  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  http_respond(request, response);

  //TODO Do some work or sleep here...
 
  // Signal completion to the database.
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("FinishWorkerTask", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(worker->workerId_).addInt32(taskId).addInt32(worker->pkey_);
  voltdb::InvocationResponse r = worker->client_->invoke(procedure);
  if (r.failure()) {
    std::cout << "FinishWorkerTask procedure failed. " << r.toString();
  }
}

void MockHTTPWorker::dispatch() {
  // Store worker pointer to access it from the request handling function.
  worker = this;

  std::cout << "Listen for HTTP requests for worker " << workerId_ << "\n";
  struct http_server_s* server = http_server_init(9090 + workerId_,
		                                  handle_request);
  http_server_listen_poll(server);
  do {
    http_server_poll(server);
  } while (!stopDispatch_);
}

void MockHTTPWorker::execute(int execId) {
  // No-op for HTTP server.
}
