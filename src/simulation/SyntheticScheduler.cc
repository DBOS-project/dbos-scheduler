// This synthetic multi-thread scheduler will create "fake" task ids, and
// queries the DB to find a worker.  There won't be actual worker or load
// generator.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel scheudlers.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "BenchmarkUtil.h"
#include "VoltdbClientUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Number of schedulers and workers
static int numSchedulers = 1;
static int numWorkers = 1;

// Assume each worker has infinite capacity.
static int workerCapacity = (1L << 27);

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int kArrayExp = 26;
static const size_t kMaxEntries = (1L << kArrayExp);

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static double* schedLatencies;
// The current index of the schedLatencies array.
static std::atomic<uint32_t> schedLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> schedIndices;
// Timestamp of each interval starts, in userc.
static std::vector<uint64_t> timeStampsUsec;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

/*
 * Scheduler thread.
 * For now, it will simply select a worker and decrease it's capacity.
 * We will need to add worker (consumer) to mark tasks finished.
 */
static void SchedulerThread(const int schedulerId,
                            const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbClientUtil::createVoltdbClient(kTestUser, kTestPwd);
  VoltdbClientUtil client(&voltdbClient, serverAddr);

  std::cout << "Scheduler: " << schedulerId << " started\n";
  do {
    auto aryIndex = schedLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array schedLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();

    // Make scheduling decisions here.
    auto workerId = client.selectWorker();
    // std::cout << "Selected: " << workerId << std::endl;

    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();
    // Record latency.
    double latency = (double)(endTime - startTime);
    schedLatencies[aryIndex] = latency;
    // TODO: maybe sleep random time for different arrival patterns.
    // std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (!mainFinished);

  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::vector<std::thread*> schedulerThreads;  // Parallel schedulers.

  // Initialize measurement arrays.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  timeStampsUsec.push_back(currTime);
  schedLatencies = new double[kMaxEntries];
  memset(schedLatencies, 0, kMaxEntries * sizeof(double));
  schedLatsArrayIndex.store(0);
  schedIndices.push_back(0);

  // Start scheduler threads.
  for (int i = 0; i < numSchedulers; ++i) {
    schedulerThreads.push_back(
        new std::thread(&SchedulerThread, i, serverAddr));
  }

  currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    std::cerr << "runBenchmark recording performance...\n";
    currTime = BenchmarkUtil::getCurrTimeUsec();
    schedIndices.push_back(schedLatsArrayIndex.load());
    timeStampsUsec.push_back(currTime);
  } while (currTime < endTime);

  mainFinished = true;
  for (int i = 0; i < numSchedulers; ++i) {
    schedulerThreads[i]->join();
    delete schedulerThreads[i];
  }

  // Processing the results.
  std::cerr << "Post processing resutls...\n";
  bool res = BenchmarkUtil::processResults(
      schedLatencies, schedIndices, timeStampsUsec, outputFile, "synthetic");
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

  // Clean up.
  delete[] schedLatencies;
  schedLatencies = nullptr;
  timeStampsUsec.clear();
  return true;
}

/*
 * Populate worker table, start from a clean state.
 */
static bool initWorkerTable(const std::string& serverAddr) {
  voltdb::Client voltdbClient =
      VoltdbClientUtil::createVoltdbClient(kTestUser, kTestPwd);
  VoltdbClientUtil client(&voltdbClient, serverAddr);

  // Clean up data from previous run.
  client.truncateWorkerTable();
  DbosStatus ret;
  for (int i = 0; i < numWorkers; ++i) {
    ret = client.insertWorker(i, workerCapacity);
    if (!ret) { return false; }
  }
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-o <output log file path>: default "
            << "synthetic_scheduler_results.csv\n";
  std::cerr << "\t-s <VoltDB master IP address>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel schedulers (threads)>: default "
            << numSchedulers << "\n";
  std::cerr << "\t-W <number of workers (#rows in table)>: default "
            << numWorkers << "\n";
  std::cerr << "\t-C <worker capacity>: default " << workerCapacity << "\n";

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("synthetic_scheduler_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "ho:s:i:t:N:W:C:")) != -1) {
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
      case 'N':
        numSchedulers = atoi(optarg);
        break;
      case 'W':
        numWorkers = atoi(optarg);
        break;
      case 'C':
        workerCapacity = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel scheduler threads: " << numSchedulers
            << "; workers: " << numWorkers << std::endl;
  std::cerr << "Worker capacity: " << workerCapacity << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  // 1) Initialize worker table in the database, add workers.
  bool res = initWorkerTable(serverAddr);
  if (!res) {
    std::cerr << "Failed to initialize worker table." << std::endl;
    exit(1);
  }

  // 2) Run experiments and parse resutls.
  res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}
