// This file contains client/loadgen code for gRPC IPC benchmarks.
#include <getopt.h>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

#include <grpcpp/grpcpp.h>

#include "ipc_bench.grpc.pb.h"
#include "BenchmarkUtil.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;
using grpc::CompletionQueue;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;

// Number of senders.
static int numSenders = 1;

// Number of broadcast receivers.
static int numReceivers = 1;

// Base receiver port.
static int basePort = 9090;

// Message size.
static int msg_size = 2048;

// Barrier used for thread synchronization.
pthread_barrier_t barrier;

// Number of parallel ping-pong messages.
static int numMessages = 1;

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int kArrayExp = 26;
static const size_t kMaxEntries = (1L << kArrayExp);

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

// Broadcast flag.
static bool broadcast = false;

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static double* msgLatencies;
// The current index of the msgLatencies array.
static std::atomic<uint32_t> msgLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> msgIndices;
// Timestamp of each interval starts, in usec.
static std::vector<uint64_t> timeStampsUsec;

// Container for the async Ping-pong call.
struct AsyncPingPongCall {
  ipcbench::StringMsg reply;
  // Context for the client. It could be used to convey extra information to
  // the server and/or tweak certain RPC behaviors.
  ClientContext context;
  // Storage for the status of the RPC upon completion
  Status status;
};

// Container for the async Broadcast call.
struct AsyncBroadcastCall {
  ipcbench::AckMsg reply;
  // Context for the client. It could be used to convey extra information to
  // the server and/or tweak certain RPC behaviors.
  ClientContext context;
  // Storage for the status of the RPC upon completion
  Status status;
};

static std::unique_ptr<ipcbench::IpcBench::Stub> addrToStub(const std::string& addr) {
  std::shared_ptr<Channel> channel =
          grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<ipcbench::IpcBench::Stub> stub =
    ipcbench::IpcBench::NewStub(channel);
  return stub;
}


// Broadcaster thread.
static void BroadcasterThread(const int serverPort, const std::string& serverAddr) {
  // Broadcast benchmark, with gRPC async client.
  // Send parallel messages to different N receivers.
  // Then wait responses of all N messages.

  std::string msgbuff(msg_size, 'a');

  // Create N stubs and connect to each gRPC server.
  std::vector<std::unique_ptr<ipcbench::IpcBench::Stub>> stubs;
  for (int i = 0; i < numReceivers; ++i) {
    // TODO: this may be an issue if we have multiple senders. Think of a
    // better way to connect to the receiver.
    stubs.push_back(addrToStub(serverAddr + ":" + std::to_string(serverPort + i)));
  }
  // Create a completion queue
  CompletionQueue cq;

  // Wait until all senders are connected.
  pthread_barrier_wait(&barrier);

  // Run a broadcast loop.
  do {
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();
    for (int i = 0; i < numReceivers; ++i) {
      // Send gRPC async request
      ipcbench::StringMsg rpcRequest;
      rpcRequest.set_senderid(i + 1);
      rpcRequest.set_msg(msgbuff);

      // Store RPC call data.
      AsyncPingPongCall* call = new AsyncPingPongCall;

      // Initiate the RPC and create a handle for it. Bind the RPC to a
      // CompletionQueue cq.
      std::unique_ptr<ClientAsyncResponseReader<ipcbench::StringMsg>> reader(
        stubs[i]->AsyncPingPong(&call->context, rpcRequest, &cq));

      // Aask for the reply and final status, with a unique tag, which is the
      // call object address.
      reader->Finish(&call->reply, &call->status, (void*)call);
    }

    // Receive all responses, drain completion queue.
    void* gotTag;
    bool rpcOk = false;
    int recvMsg = 0; // received responses.
    while((recvMsg < numReceivers) && cq.Next(&gotTag, &rpcOk)) {
      // The tag is the memory location of the call object.
      AsyncPingPongCall* call = static_cast<AsyncPingPongCall*>(gotTag);
      assert(rpcOk);
      assert(call->status.ok());
      // Make sure we received the correct length msg.
      assert(call->reply.msg().size() == msg_size);
      delete call;
      recvMsg++;
    }
    assert(recvMsg == numReceivers);
    
    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();

    // Record the latency.
    auto aryIndex = msgLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array msgLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    msgLatencies[aryIndex] = double(endTime - startTime);
  } while (!mainFinished);

  // Clean up
  cq.Shutdown();

  return;

}

// Ping-pong sender thread.
static void SenderThread(const int serverPort, const std::string& serverAddr) {
  // Ping-pong benchmark, with gRPC async client.
  // Send M outstanding message to a single receiver.
  // Then wait responses of all M messages.

  std::string msgbuff(msg_size, 'a');

  // Create stub and connect to the gRPC server.
  std::unique_ptr<ipcbench::IpcBench::Stub> stub =
    addrToStub(serverAddr + ":" + std::to_string(serverPort));

  // Create a completion queue
  CompletionQueue cq;

  // Wait until all senders are connected.
  pthread_barrier_wait(&barrier);

  // Run a ping-pong loop.
  do {
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();
    for (int i = 0; i < numMessages; ++i) {
      // Send gRPC async request
      ipcbench::StringMsg rpcRequest;
      rpcRequest.set_senderid(serverPort - basePort + 1);
      rpcRequest.set_msg(msgbuff);

      // Store RPC call data.
      AsyncPingPongCall* call = new AsyncPingPongCall;

      // Initiate the RPC and create a handle for it. Bind the RPC to a
      // CompletionQueue cq.
      std::unique_ptr<ClientAsyncResponseReader<ipcbench::StringMsg>> reader(
        stub->AsyncPingPong(&call->context, rpcRequest, &cq));

      // Aask for the reply and final status, with a unique tag, which is the
      // call object address.
      reader->Finish(&call->reply, &call->status, (void*)call);
    }

    // Receive all responses, drain completion queue.
    void* gotTag;
    bool rpcOk = false;
    int recvMsg = 0; // received responses.
    while((recvMsg < numMessages) && cq.Next(&gotTag, &rpcOk)) {
      // The tag is the memory location of the call object.
      AsyncPingPongCall* call = static_cast<AsyncPingPongCall*>(gotTag);
      assert(rpcOk);
      assert(call->status.ok());
      // Make sure we received the correct length msg.
      assert(call->reply.msg().size() == msg_size);
      delete call;
      recvMsg++;
    }
    assert(recvMsg == numMessages);
    
    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();

    // Record the latency.
    auto aryIndex = msgLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array msgLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    msgLatencies[aryIndex] = double(endTime - startTime);
  } while (!mainFinished);

  // Clean up
  cq.Shutdown();

  return;
}

// Actually run the benchmark, main thread loop.
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::vector<std::thread*> senderThreads;  // Parallel senders.
  std::string benchName = "gRPC";

  // Initialize measurement arrays.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  timeStampsUsec.push_back(currTime);
  msgLatencies = new double[kMaxEntries];
  memset(msgLatencies, 0, kMaxEntries * sizeof(double));
  msgLatsArrayIndex.store(0);
  msgIndices.push_back(0);

  // Initialize barrier.
  pthread_barrier_init(&barrier, NULL, numSenders);

  // Start sender threads.
  if (broadcast) {
    benchName = benchName + "-broadcast";
    for (int i = 0; i < numSenders; ++i) {
      senderThreads.push_back(new std::thread(&BroadcasterThread, basePort + i,
                                              serverAddr));
    }
  } else {
    benchName = benchName + "-pingpong";
    for (int i = 0; i < numSenders; ++i) {
      senderThreads.push_back(new std::thread(&SenderThread, basePort + i,
                                              serverAddr));
    }
  }

  currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    std::cerr << "runBenchmark recording performance...\n";
    currTime = BenchmarkUtil::getCurrTimeUsec();
    msgIndices.push_back(msgLatsArrayIndex.load());
    timeStampsUsec.push_back(currTime);
  } while (currTime < endTime);

  mainFinished = true;
  for (int i = 0; i < numSenders; ++i) {
    senderThreads[i]->join();
    delete senderThreads[i];
  }

  // Processing the results.
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      msgLatencies, msgIndices, timeStampsUsec, outputFile, benchName);
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

  // Clean up.
  delete[] msgLatencies;
  msgLatencies = nullptr;
  timeStampsUsec.clear();
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-b: run broadcast benchmark\n";
  std::cerr << "\t-o <output log file path>: default "
            << "grpc_bench_results.csv\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-p <port number>: default " << basePort << std::endl;
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel senders (threads)>: default "
            << numSenders << "\n";
  std::cerr << "\t-M <number of parallel ping-pong messages>: default "
            << numMessages << "\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << "\t-R <number of receivers> used only when broadcasting: default "
            << numReceivers << "\n";

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("grpc_bench_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hbo:s:p:i:t:N:M:m:R:")) != -1) {
    switch (opt) {
      case 'o':
        outputFile = optarg;
        break;
      case 's':
        serverAddr = optarg;
        break;
      case 'p':
        basePort = atoi(optarg);
        break;
      case 'i':
        measureIntervalMsec = atoi(optarg);
        break;
      case 't':
        totalExecTimeMsec = atoi(optarg);
        break;
      case 'N':
        numSenders = atoi(optarg);
        break;
      case 'M':
        numMessages = atoi(optarg);
        break;
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'b':
        broadcast = true;
        break;
      case 'R':
        numReceivers = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel sender threads: " << numSenders << std::endl;
  std::cerr << "Parallel outstanding messages: " << numMessages << std::endl;
  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "Server address: " << serverAddr << std::endl;
  std::cerr << "Base port: " << basePort << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";
  if (broadcast) {
    std::cerr << "Broadcasting to " << numReceivers << " receivers\n";
  }

  // Run experiments and parse results.
  bool res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}
