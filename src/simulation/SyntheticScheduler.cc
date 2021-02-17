// This synthetic multi-thread scheduler will create "fake" task ids, and
// queries the DB to find a worker.  There won't be actual worker or load
// generator.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel scheudlers.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "BenchmarkUtil.h"
#include "PartitionedFIFOScheduler.h"
#include "PartitionedLocalFIFOScheduler.h"
#include "PartitionedFIFOTaskScheduler.h"
#include "PartitionedScanTask.h"
#include "PushFIFOScheduler.h"
#include "RandomGenerator.h"
#include "SinglePartitionedFIFOTaskScheduler.h"
#include "SparkScheduler.h"
#include "VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Number of schedulers and workers
static int numSchedulers = 1;
static int numWorkers = 1;
static int numTasks = 8;

// Number of worker/task partitions, not VoltDB partitions.
static int partitions = 8;

// Assume each worker has infinite capacity.
static int workerCapacity = (1L << 27);

// Probability to do multi-partition transaction
static float probMultiTx = 0.0;

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int kArrayExp = 26;
static const size_t kMaxEntries = (1L << kArrayExp);

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

// Requests per second, must be greater than 0.
static double reqPerSec = 100;
static double reqPerSecThread = 100;  // Request rate per thread.

// Types of random distributions.
static const std::string kNoDist = "none";
static const std::string kFixed = "fixed";
static const std::string kPoisson = "poisson";
static const std::string kUniform = "uniform";
static const std::unordered_set<std::string> kDists = {kNoDist, kFixed,
                                                       kPoisson, kUniform};
static std::string reqDist = kNoDist;  // by default, send as fast as possible.

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

// Type of scheduler algorithm
// TODO: add more types here.
static const std::string kFifoAlgo = "fifo";
static const std::string kFifoLocalAlgo = "fifo-local";
static const std::string kFifoTaskAlgo = "fifo-task";
static const std::string kSinglePartitionedFifoTaskAlgo =
    "single-partitioned-fifo-task";
static const std::string kSparkAlgo = "spark";
static const std::string kScanTaskAlgo = "scan-task";
static const std::string kPushFifoAlgo = "push-fifo";
static const std::unordered_set<std::string> kAlgorithms = {
    kFifoAlgo,  kFifoTaskAlgo, kSinglePartitionedFifoTaskAlgo,
    kSparkAlgo, kScanTaskAlgo, kPushFifoAlgo};
static std::string scheduleAlgo = kFifoAlgo;

// If true, truncate tables after execution.
static bool cleanDB = false;

/*
 * Return a constructed scheduler instance based on algorithm.
 */
static VoltdbSchedulerUtil* constructScheduler(voltdb::Client* voltdbClient,
                                               const std::string& serverAddr,
                                               const std::string& algo) {
  VoltdbSchedulerUtil* scheduler = nullptr;
  if (algo == kFifoAlgo) {
    scheduler = new PartitionedFIFOScheduler(
        voltdbClient, serverAddr, partitions, workerCapacity, numWorkers);
  } else if (algo == kFifoLocalAlgo) {
    scheduler = new PartitionedLocalFIFOScheduler(
        voltdbClient, serverAddr, partitions, workerCapacity, numWorkers);
  } else if (algo == kFifoTaskAlgo) {
    scheduler = new PartitionedFIFOTaskScheduler(
        voltdbClient, serverAddr, partitions, numTasks, workerCapacity,
        numWorkers, probMultiTx);
  } else if (algo == kSinglePartitionedFifoTaskAlgo) {
    scheduler = new SinglePartitionedFIFOTaskScheduler(
        voltdbClient, serverAddr, partitions, numTasks, workerCapacity,
        numWorkers, probMultiTx);
  } else if (algo == kSparkAlgo) {
    scheduler = new SparkScheduler(voltdbClient, serverAddr, partitions,
                                   workerCapacity, numWorkers);
  } else if (algo == kScanTaskAlgo) {
    scheduler = new PartitionedScanTask(voltdbClient, serverAddr, partitions,
                                        numTasks, numWorkers, probMultiTx);
  } else if (algo == kPushFifoAlgo) {
    scheduler = new PushFIFOScheduler(voltdbClient, serverAddr, partitions,
                                      numTasks, probMultiTx);
  } else {
    std::cerr << "Unsupported scheduler algorithm: " << algo << "\n";
  }

  return scheduler;
}

/*
 * Scheduler thread.
 * For now, it will simply select a worker and decrease it's capacity.
 * We will need to add worker (consumer) to mark tasks finished.
 */
static void SchedulerThread(const int schedulerId,
                            const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd);

  VoltdbSchedulerUtil* scheduler =
      constructScheduler(&voltdbClient, serverAddr, scheduleAlgo);
  assert(scheduler != nullptr);
  std::cout << "Scheduler: " << schedulerId << " started\n";

  // Inter-arrival latency generator.
  Generator* iaGen = nullptr;
  if (reqDist == kFixed) {
    iaGen = new Fixed(reqPerSecThread);
  } else if (reqDist == kPoisson) {
    iaGen = new Poisson(reqPerSecThread);
  } else if (reqDist == kUniform) {
    iaGen = new Uniform(reqPerSecThread);
  } else if (reqDist == kNoDist) {
    std::cerr << "sending as fast as possible.\n";
  } else {
    std::cerr << "Unsupported distribution: " << reqDist << "\n";
    exit(-1);
  }

  // If No distribution, send out as fast as possible.
  // Otherwise, send request based on the given request rate and distribution.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t nextTime = currTime;
  if (iaGen != nullptr) {
    nextTime = currTime + (uint64_t)(iaGen->generate() * 1000 * 1000);
  }

  do {
    // Not its turn to send the request.
    if ((iaGen != nullptr) && (nextTime > currTime)) {
      // Avoid busy loop. We also don't want to directly sleep interval time
      // because that may cause high latency or cannot finish the experiment
      // in time.
      uint64_t delay = (nextTime - currTime);
      delay = delay > 5 ? 5 : delay;
      std::this_thread::sleep_for(std::chrono::microseconds(delay));
      currTime = BenchmarkUtil::getCurrTimeUsec();
      continue;
    }

    // Otherwise, send the request and record latency.
    auto aryIndex = schedLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array schedLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();

    // Make scheduling decisions here.
    // TODO:  Create a "Task for scheduler" function parametrized by workload.
    Task* task = new Task;
    task->targetData = rand() % numWorkers;
    task->execTime = 1000;
    auto status = scheduler->schedule(task);
    assert(status);
    delete task;

    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();
    // Record latency.
    double latency = (double)(endTime - startTime);
    schedLatencies[aryIndex] = latency;

    // Update nextTime.
    if (iaGen != nullptr) {
      nextTime = nextTime + (uint64_t)(iaGen->generate() * 1000 * 1000);
    }

    // Update currTime.
    currTime = endTime;

  } while (!mainFinished);

  sleep(1);  // Give outstanding requests time to finish.

  // Clean up
  delete scheduler;
  if (iaGen != nullptr) { delete iaGen; }
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
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      schedLatencies, schedIndices, timeStampsUsec, outputFile, scheduleAlgo);
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
 * Setup database.
 */
static bool setup(const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd);

  VoltdbSchedulerUtil* scheduler =
      constructScheduler(&voltdbClient, serverAddr, scheduleAlgo);
  assert(scheduler != nullptr);
  bool res = scheduler->setup();
  delete scheduler;
  return res;
}

/*
 * Cleanup database.
 */
static bool teardown(const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd);

  VoltdbSchedulerUtil* scheduler =
      constructScheduler(&voltdbClient, serverAddr, scheduleAlgo);
  assert(scheduler != nullptr);
  bool res = scheduler->teardown();
  delete scheduler;
  return res;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-x: truncate DB tables after execution.\n";
  std::cerr << "\t-o <output log file path>: default "
            << "synthetic_scheduler_results.csv\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel schedulers (threads)>: default "
            << numSchedulers << "\n";
  std::cerr << "\t-W <number of workers (#rows in table)>: default "
            << numWorkers << "\n";
  std::cerr << "\t-C <worker capacity>: default " << workerCapacity << "\n";
  std::cerr << "\t-T <number of tasks>: default " << numTasks << "\n";
  std::cerr << "\t-P <partitions>: default " << partitions << "\n";
  std::cerr << "\t-R <request rate>: default " << reqPerSec << "\n";
  std::cerr << "\t-D <request distribution>: default " << reqDist
            << " (options: ";
  for (auto&& it : kDists) { std::cerr << it << " "; }
  std::cerr << ")\n";
  std::cerr
      << "\t-p <probability of multi-partition transaction> (0-1.0): default "
      << probMultiTx << "\n";
  // Print all options here.
  std::cerr << "\t-A <scheduler algorithm (options: ";
  for (auto&& it : kAlgorithms) { std::cerr << it << " "; }
  std::cerr << ")> default " << scheduleAlgo << "\n";

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("synthetic_scheduler_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hxo:s:i:t:N:W:C:P:A:T:p:R:D:")) != -1) {
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
      case 'P':
        partitions = atoi(optarg);
        break;
      case 'A':
        scheduleAlgo = optarg;
        break;
      case 'T':
        numTasks = atoi(optarg);
        break;
      case 'p':
        probMultiTx = atof(optarg);
        break;
      case 'R':
        reqPerSec = atof(optarg);
        break;
      case 'D':
        reqDist = optarg;
        break;
      case 'x':
        cleanDB = true;
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  // Check scheduler algorithm type valid here.
  auto algoIt = kAlgorithms.find(scheduleAlgo);
  if (algoIt == kAlgorithms.end()) {
    std::cerr << "Unsupported algorithm: " << scheduleAlgo << std::endl;
    Usage(argv);
  }
  std::cerr << "Scheduler algorithm: " << scheduleAlgo << std::endl;
  std::cerr << "Probability of multi-partition transaction: " << probMultiTx
            << std::endl;
  std::cerr << "Parallel scheduler threads: " << numSchedulers
            << "; workers: " << numWorkers << "; tasks: " << numTasks
            << std::endl;
  std::cerr << "Worker capacity: " << workerCapacity << std::endl;
  std::cerr << "Partitions: " << partitions << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  auto distIt = kDists.find(reqDist);
  if (distIt == kDists.end()) {
    std::cerr << "Unsupported distribution type: " << reqDist << "\n";
    Usage(argv);
  }
  if (reqDist != kNoDist) {
    std::cerr << "Request per sec: " << reqPerSec;
    std::cerr << "; Distribution: " << reqDist << "\n";

    // Calculate per scheduler thread request rate.
    reqPerSecThread = reqPerSec / numSchedulers;
  } else {
    std::cerr << "No distribution type, send as fast as possible.\n";
  }

  // 1) Initialize database state.
  bool res = setup(serverAddr);
  if (!res) {
    std::cerr << "Failed to initialize database state." << std::endl;
    exit(1);
  }

  // 2) Run experiments and parse results.
  res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  // 3) Clean database state.
  if (cleanDB) {
    res = teardown(serverAddr);
    if (!res) {
      std::cerr << "Failed to tear down database state." << std::endl;
      exit(1);
    }
  }

  return 0;
}
