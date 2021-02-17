// This **asynchronous** synthetic multi-thread scheduler will create
// "fake" task ids, and query the DB to find a worker.  There won't be actual
// workers or load generators.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel schedulers.

#include <getopt.h>
#include <atomic>
#include <boost/shared_ptr.hpp>
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
#include "voltdb-client-cpp/include/ProcedureCallback.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"

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

// TODO: currently only FIFO supports async. Will add more.
static const std::unordered_set<std::string> kAlgorithms = {kFifoAlgo, kFifoLocalAlgo};
static std::string scheduleAlgo = kFifoAlgo;

// Max outstanding requests per thread.
static int maxOutstanding = 1;

// If true, truncate tables after execution.
static bool cleanDB = false;

// If true, do not setup tables before execution. Otherwise, setup the table.
static bool noSetupDB = false;

// If true, output avg cpu usage for each benchmark
static bool output_cpu_usage = false;
static const std::string cpu_usage_log = "cpu_usage.txt";

// Callback for async client.
// Note: VoltDB C++ client is single threaded; thus the callback is executed on
// the same thread as the one sending requests. Therefore, the sender thread
// needs to periodically call "runOnce()" to enter the event loop and process
// callbacks.
class SchedulerCallback : public voltdb::ProcedureCallback {
public:
  SchedulerCallback(int64_t maxOutCnt)
      : outCnt_(0), endThresh_(maxOutCnt / 2) {}

  bool callback(voltdb::InvocationResponse response) throw(voltdb::Exception) {
    bool retVal = false;
    if (response.failure()) {
      std::cerr << "Failed to execute!\n";
      std::cerr << response.toString();
      exit(1);
    }
    auto aryIndex = schedLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array schedLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }

    // Convert to microseconds.
    // NOTE: the clusterRoundTripTime is the VoltDB internal server side
    // latency, and in millisecond granularity. Thus, it is normal to have
    // "zero" latency here.
    // TODO: figure out a better way to measure async client side latency.
    double latency = (double)response.clusterRoundTripTime() * 1000.0;
    schedLatencies[aryIndex] = latency;
    std::vector<voltdb::Table> results = response.results();
    voltdb::Row row = results[0].iterator().next();
    DbosId selectedWorker = row.getInt64(0);
    assert(selectedWorker >= 0);

    outCnt_--;
    if (outCnt_ <= endThresh_) {
      // If reaches the threshold, break the event loop and return to
      // scheduling. Otherwise, the event loop would continue to process all
      // responses.
      // TODO: find a better heuristic to end the loop.
      retVal = true;
    }
    return retVal;
  }

  int64_t outCnt_;

private:
  int64_t endThresh_;  // threshold to end the event loop.
};

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
  boost::shared_ptr<SchedulerCallback> callback(
      new SchedulerCallback(maxOutstanding));
  callback->outCnt_ = 0;
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

  // If No distribution, send out as fast as possible, and allow batching.
  if (reqDist == kNoDist) {
    do {
      // Make async scheduling decisions here.
      auto status = scheduler->asyncSchedule(callback);
      assert(status);
      callback->outCnt_++;
      if (callback->outCnt_ >= maxOutstanding) {
        // Run the event loop once; return immediately if no responses. It will
        // process callbacks until the event loop finished or callback returns
        // true.
        // TODO: either find a better heuristic, or run callbacks on a separate
        // thread.
        voltdbClient.runOnce();
        continue;
      }

      if (callback->outCnt_ > maxOutstanding / 2) {
        // Heuristic to process responses in time.
        // TODO: either find a better heuristic, or run callbacks on a separate
        // thread.
        voltdbClient.runOnce();
        continue;
      }

    } while (!mainFinished);

  } else {
    // Otherwise, enter the control loop, execute the query without batching.
    uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
    uint64_t nextTime = currTime + (uint64_t)(iaGen->generate() * 1000 * 1000);
    do {
      if (nextTime <= currTime) {
        // Make async scheduling decisions here.
        auto status = scheduler->asyncSchedule(callback);
        assert(status);
        callback->outCnt_++;
        // Run the event loop once; return immediately if no responses.
        voltdbClient.runOnce();

        // Update nextTime.
        nextTime = nextTime + (uint64_t)(iaGen->generate() * 1000 * 1000);
      } else {
        // Avoid busy loop. We also don't want to directly sleep interval time
        // because that may cause high latency or cannot finish the experiment
        // in time.
        uint64_t delay = (nextTime - currTime);
        delay = delay > 5 ? 5 : delay;
        std::this_thread::sleep_for(std::chrono::microseconds(delay));
      }
      currTime = BenchmarkUtil::getCurrTimeUsec();

    } while (!mainFinished);
  }

  // Give outstanding requests time to finish.
  while (!voltdbClient.drain()) {}

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

  std::string cmd;
  if (output_cpu_usage) {
    std::string log_header_cmd = "echo | date > " + cpu_usage_log;
    std::system(log_header_cmd.c_str());
    cmd = "top -b -n 2 -d " + std::to_string((float) measureIntervalMsec / 1000.0) + 
              " | awk '/top - /{i++}i>=2'" + ">> "+ cpu_usage_log + " &";
  }

  currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    if (output_cpu_usage) std::system(cmd.c_str());
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
  std::cerr << "\t-X: NOT setup DB tables before execution.\n";
  std::cerr << "\t-c: output log of avg cpu usage.\n";
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
  // Max outstanding requests: if reaches this threshold, runs the event loop
  // once to process potential responses.
  std::cerr << "\t-m <max outstanding requests>: default " << maxOutstanding
            << "\n";
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
  std::string cpuUsageOutputFile("cpu_usage.txt");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hxXco:s:i:t:N:W:C:P:A:T:p:R:D:m:")) != -1) {
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
      case 'X':
        noSetupDB = true;
        break;
      case 'm':
        maxOutstanding = atoi(optarg);
        break;
      case 'c':
        output_cpu_usage = true;
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
  std::cerr << "Maximum outstanding requests: " << maxOutstanding << "\n";

  // 1) Initialize database state.
  bool res = false;

  if (!noSetupDB) {
    res = setup(serverAddr);
    if (!res) {
      std::cerr << "Failed to initialize database state." << std::endl;
      exit(1);
    }
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
