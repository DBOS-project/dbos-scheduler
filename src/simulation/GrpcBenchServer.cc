// This file contains server/receiver code for gRPC IPC benchmarks.

#include <getopt.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "ipc_bench.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;

// Number of receivers.
static int numReceivers = 1;

// Base receiver port.
static int basePort = 9090;

namespace ipcbench {

class IpcBenchImpl final : public IpcBench::Service {
public:
  IpcBenchImpl() {}

  ~IpcBenchImpl() {}

private:
  Status PingPong(ServerContext* context, const StringMsg* request,
                  StringMsg* reply) override;

  Status Broadcast(ServerContext* context, const StringMsg* request,
                   AckMsg* reply) override;

};  // class IpcBenchImpl

// Receive a Ping-pong msg from the client.
Status IpcBenchImpl::PingPong(ServerContext* context, const StringMsg* request,
                              StringMsg* reply) {
  // std::cout << "Received ping-pong from sender: " << request->senderid() <<
  // ", msg: "
  //          << request->msg() << std::endl;
  reply->set_senderid(request->senderid());
  reply->set_msg(request->msg());
  return Status::OK;
}

// Receive a broadcast msg from the client.
Status IpcBenchImpl::Broadcast(ServerContext* context, const StringMsg* request,
                               AckMsg* reply) {
  std::cout << "Received broadcast from sender: " << request->senderid()
            << ", msg: " << request->msg() << std::endl;
  reply->set_ack(true);
  return Status::OK;
}

}  // namespace ipcbench

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  // std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << "\t-p <base port>: default " << basePort << std::endl;
  std::cerr << "\t-N <number of parallel receivers (threads)>: default "
            << numReceivers << "\n";
  // Print all options here.

  std::cerr << std::endl;
  exit(1);
}

// Receiver thread.
static void ReceiverThread(const int serverPort) {
  // Start a gRPC server.
  std::string addr = "0.0.0.0:" + std::to_string(serverPort);
  ipcbench::IpcBenchImpl service;
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Set max message size.
  builder.SetMaxMessageSize(INT32_MAX);

  // Finally, assemble the server.
  std::unique_ptr<Server> recvServer = builder.BuildAndStart();
  std::cout << "Started gRPC server on addr " << addr << std::endl;
  recvServer->Wait();
}

int main(int argc, char** argv) {
  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hp:N:")) != -1) {
    switch (opt) {
      case 'p':
        basePort = atoi(optarg);
        break;
      case 'N':
        numReceivers = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel receiver threads: " << numReceivers << std::endl;
  std::cerr << "Base port: " << basePort << std::endl;

  std::vector<std::thread*> receiverThreads;  // Parallel receivers.

  // Start scheduler threads.
  for (int i = 0; i < numReceivers; ++i) {
    receiverThreads.push_back(new std::thread(&ReceiverThread, basePort + i));
  }

  // Sleep forever.
  while (1) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }

  return 0;
}
