// This synthetic multi-thread worker will create workers, and waiting for
// assigned tasks.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "simulation/BenchmarkUtil.h"
#include "simulation/VoltdbWorkerUtil.h"
#include "simulation/MockPollWorker.h"
#include "voltdb-client-cpp/include/Client.h"

// Number of workers
static int numWorkers = 1;

// Number of executors per worker
static int numExecutors = 1;

// Number of worker/task partitions, not VoltDB partitions.
static int partitions = 8;

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

static bool mainFinished = false;  // Control whether to stop the experiment.

// Type of worker
// TODO: add more types here.
static const std::string kMockPoll = "mock-poll";  // Polling DB to find executable tasks.
static const std::unordered_set<std::string> kWorkerTypes = {kMockPoll};
static std::string workerType = kMockPoll;

/*
 * Return a constructed worker instance based on type.
 */
static VoltdbWorkerUtil* constructWorker(const DbosId workerId,
                                         const std::string& serverAddr,
                                         const std::string& type) {
  VoltdbWorkerUtil* worker = nullptr;
  if (type == kMockPoll) {
    int pkey = workerId % partitions;
    worker = new MockPollWorker(workerId, pkey, serverAddr, numExecutors);
  } else {
    std::cerr << "Unsupported worker type: " << type << "\n";
  }

  return worker;
}

/*
 * Worker thread.
 */
static void WorkerThread(const int workerId,
                         const std::string& serverAddr) {
  VoltdbWorkerUtil* worker = constructWorker(workerId, serverAddr, workerType);
  assert(worker != nullptr);
  // TODO: implement launching executor threads, and dispatch thread.
  worker->setup();
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  } while (!mainFinished);
  // Clean up
  worker->teardown();
  delete worker;
  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::vector<std::thread*> workerThreads;  // Parallel workers.

  // Start worker threads.
  for (int i = 0; i < numWorkers; ++i) {
    workerThreads.push_back(
        new std::thread(&WorkerThread, i, serverAddr));
  }

  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    std::cerr << "runBenchmark recording performance...\n";
    // TODO: implement performance benchmarking.
    currTime = BenchmarkUtil::getCurrTimeUsec();
  } while (currTime < endTime);

  mainFinished = true;
  for (int i = 0; i < numWorkers; ++i) {
    workerThreads[i]->join();
    delete workerThreads[i];
  }

  // TODO: Processing the results.

  // Clean up.
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-o <output log file path>: default "
            << "synthetic_worker_results.csv\n";
  std::cerr << "\t-s <VoltDB master IP address>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-W <number of workers (#rows in table)>: default "
            << numWorkers << "\n";
  std::cerr << "\t-P <partitions>: default " << partitions << "\n";
  // Print all options here.
  std::cerr << "\t-A <worker type (options: ";
  for (auto&& it : kWorkerTypes) { std::cerr << it << " "; }
  std::cerr << ")> default " << workerType << "\n";

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("synthetic_worker_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "ho:s:i:t:W:P:A:")) != -1) {
    switch (opt) {
      case 'o':
        outputFile = optarg;
        break;
      case 's':
        serverAddr = optarg;
        break;
      case 'i':
        measureIntervalMsec = atoi(optarg);
        break;
      case 't':
        totalExecTimeMsec = atoi(optarg);
        break;
      case 'W':
        numWorkers = atoi(optarg);
        break;
      case 'P':
        partitions = atoi(optarg);
        break;
      case 'A':
        workerType = optarg;
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  // Check worker type valid here.
  auto algoIt = kWorkerTypes.find(workerType);
  if (algoIt == kWorkerTypes.end()) {
    std::cerr << "Unsupported worker type: " << workerType << std::endl;
    Usage(argv);
  }
  std::cerr << "Worker type: " << workerType << std::endl;
  std::cerr << "Parallel workers: " << numWorkers << std::endl;
  std::cerr << "Partitions: " << partitions << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  // Run experiments and parse results.
  bool res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}