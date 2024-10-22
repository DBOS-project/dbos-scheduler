// This synthetic multi-thread worker will create workers, and waiting for
// assigned tasks.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "BenchmarkUtil.h"
#include "MockHTTPWorker.h"
#include "MockPollWorker.h"
#include "WorkerManager.h"
#include "voltdb-client-cpp/include/Client.h"

// Number of workers
static int numWorkers = 1;

// Number of executors per worker
static int numExecutors = 1;

// Number of worker/task partitions, not VoltDB partitions.
static int partitions = 8;

// Select top-k tasks at a time.
static int topkTasks = 1;

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

static bool mainFinished = false;  // Control whether to stop the experiment.
static std::mutex mainLock;        // For the condition variable.
static std::condition_variable
    mainCv;  // Condition variable that notifies worker control threads.

// Type of worker
// TODO: add more types here.
static const std::string kMockPoll =
    "mock-poll";  // Polling DB to find executable tasks.
static const std::string kMockHTTP =
    "mock-http";  // Listen for HTTP requests to execute.
static const std::unordered_set<std::string> kWorkerTypes = {kMockPoll,
                                                             kMockHTTP};
static std::string workerType = kMockPoll;

// Record performance
static std::vector<double> dispatchThroughput;
static std::vector<double> finishThroughput;

/*
 * Return a constructed worker instance based on type.
 */
static WorkerManager* constructWorker(voltdb::Client* voltdbClient,
                                      const DbosId workerId,
                                      const std::string& serverAddr,
                                      const std::string& type) {
  WorkerManager* worker = nullptr;
  if (type == kMockPoll) {
    int pkey = workerId % partitions;
    worker =
        new MockPollWorker(workerId, pkey, serverAddr, numExecutors, topkTasks);
  } else if (type == kMockHTTP) {
    int pkey = workerId % partitions;
    worker = new MockHTTPWorker(voltdbClient, workerId, pkey, serverAddr);
  } else {
    std::cerr << "Unsupported worker type: " << type << "\n";
  }

  return worker;
}

/*
 * Worker thread.
 */
static void WorkerThread(const int workerId, const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient = WorkerManager::createVoltdbClient(serverAddr);

  WorkerManager* worker =
      constructWorker(&voltdbClient, workerId, serverAddr, workerType);
  assert(worker != nullptr);
  worker->startServing();

  {
    // Wait for main thread finish signal.
    std::unique_lock<std::mutex> lock(mainLock);
    mainCv.wait(lock, [] { return mainFinished; });
    lock.unlock();
  }

  // Clean up
  worker->endServing();
  delete worker;
  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;
  WorkerManager::totalTasks_.store(0);
  WorkerManager::totalFinishedTasks_.store(0);
  std::vector<std::thread*> workerThreads;  // Parallel workers.

  // Start worker threads.
  for (int i = 0; i < numWorkers; ++i) {
    workerThreads.push_back(new std::thread(&WorkerThread, i, serverAddr));
  }

  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t lastTime = currTime;
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  uint64_t lastTasks = WorkerManager::totalTasks_.load();
  uint64_t lastFinished = WorkerManager::totalFinishedTasks_.load();
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    std::cerr << "runBenchmark recording performance...\n";
    // TODO: implement performance benchmarking.
    uint64_t currTasks = WorkerManager::totalTasks_.load();
    uint64_t currFinished = WorkerManager::totalFinishedTasks_.load();
    currTime = BenchmarkUtil::getCurrTimeUsec();

    // Compute throughput
    double currThroughput =
        (currTasks - lastTasks) * 1.0 / ((currTime - lastTime) / 1000000.0);
    double currFinishedThroughput = (currFinished - lastFinished) * 1.0 /
                                    ((currTime - lastTime) / 1000000.0);
    dispatchThroughput.push_back(currThroughput);
    finishThroughput.push_back(currFinishedThroughput);
    lastTasks = currTasks;
    lastTime = currTime;
    lastFinished = currFinished;
  } while (currTime < endTime);

  // Notifying workers to stop.
  mainFinished = true;
  mainCv.notify_all();
  for (int i = 0; i < numWorkers; ++i) {
    workerThreads[i]->join();
    delete workerThreads[i];
  }

  // TODO: Processing the results.
  std::cerr << "Dispatch-Throughput,Finished-Throughput\n";
  for (int i = 0; i < dispatchThroughput.size(); ++i) {
    std::cerr << dispatchThroughput[i] << ",  " << finishThroughput[i]
              << std::endl;
  }

  // Clean up.
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-o <output log file path>: default "
            << "synthetic_worker_results.csv\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-W <number of workers (#rows in table)>: default "
            << numWorkers << "\n";
  std::cerr << "\t-E <number of executors per worker>: default " << numExecutors
            << "\n";
  std::cerr << "\t-K <select top-K tasks in a batch>: default " << topkTasks
            << "\n";

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
  while ((opt = getopt(argc, argv, "ho:s:i:t:W:P:A:E:K:")) != -1) {
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
      case 'E':
        numExecutors = atoi(optarg);
        break;
      case 'K':
        topkTasks = atoi(optarg);
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
  std::cerr << "Executors per worker: " << numExecutors << std::endl;
  std::cerr << "Task top-k (batch) size: " << topkTasks << std::endl;
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
