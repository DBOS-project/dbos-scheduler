// This **asynchronous** communication benchmark will create
// "fake" messages and insert them to the DB. There won't be actual receivers.
// The goal is to benchmark the basic latency, throughput, and scalability of
// different communication methods.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "BenchmarkUtil.h"
#include "PartitionedFIFOScheduler.h"
#include "PartitionedFIFOTaskScheduler.h"
#include "PartitionedScanTask.h"
#include "PushFIFOScheduler.h"
#include "SinglePartitionedFIFOTaskScheduler.h"
#include "SparkScheduler.h"
#include "VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ProcedureCallback.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"

// Number of senders.
static int numSenders = 1;

// Number of workers and tasks.
// Exists for compatibility with the scheduler API.
static int numWorkers = 1;
static int numTasks = 8;

// Number of partitions.
static int partitions = 8;

// Assume each worker has infinite capacity.
// Exists for compatibility with the scheduler API.
static int workerCapacity = (1L << 27);

// Probability to do multi-partition transaction.
// Exists for compatibility with the scheduler API.
static float probMultiTx = 0.0;

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int kArrayExp = 26;
static const size_t kMaxEntries = (1L << kArrayExp);

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

// Wait interval between requests.
static int arrivalDelay = 0;

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static double* msgLatencies;
// The current index of the schedLatencies array.
static std::atomic<uint32_t> msgLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> msgIndices;
// Timestamp of each interval starts, in userc.
static std::vector<uint64_t> timeStampsUsec;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

// Type of scheduler algorithm.
// Exists for compatibility with the scheduler API.
static const std::string kFifoAlgo = "fifo";
static const std::string kFifoTaskAlgo = "fifo-task";
static const std::string kSinglePartitionedFifoTaskAlgo =
    "single-partitioned-fifo-task";
static const std::string kSparkAlgo = "spark";
static const std::string kScanTaskAlgo = "scan-task";
static const std::string kPushFifoAlgo = "push-fifo";

// Exists for compatibility with the scheduler API.
static const std::unordered_set<std::string> kAlgorithms = {
    kFifoAlgo};
static std::string scheduleAlgo = kFifoAlgo;

// Max outstanding requests per thread.
static int maxOutstanding = 1;

// If true, truncate tables after execution.
static bool cleanDB = false;

// If true, do not setup tables before execution. Otherwise, setup the table.
static bool noSetupDB = false;

// If true, use a VoltDB replicated table to broadcast a message.
static bool broadcast = false;

// Callback for async client.
// Note: VoltDB C++ client is single threaded; thus the callback is executed on
// the same thread as the one sending requests. Therefore, the sender thread
// needs to periodically call "runOnce()" to enter the event loop and process
// callbacks.
class SenderCallback: public voltdb::ProcedureCallback {
public:
  SenderCallback(int64_t maxOutCnt): outCnt_(0), endThresh_(maxOutCnt/2) {}

  bool callback(voltdb::InvocationResponse response) throw (voltdb::Exception) {
    bool retVal = false;
    if (response.failure()) {
      std::cerr << "Failed to execute!\n";
      std::cerr << response.toString();
      exit(1);
    }
    auto aryIndex = msgLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array msgLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }

    // Convert to microseconds.
    // NOTE: the clusterRoundTripTime is the VoltDB internal server side
    // latency, and in millisecond granularity. Thus, it is normal to have
    // "zero" latency here.
    // TODO: figure out a better way to measure async client side latency.
    double latency = (double)response.clusterRoundTripTime() * 1000.0;
    msgLatencies[aryIndex] = latency;

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
 * Sender thread.
 * For now, it will simply insert messages to the DB.
 */
static void SenderThread(const int schedulerId,
                         const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::Client voltdbClient =
      VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd);

  VoltdbSchedulerUtil* scheduler =
      constructScheduler(&voltdbClient, serverAddr, scheduleAlgo);
  assert(scheduler != nullptr);
  std::cout << "Sender: " << schedulerId << " started\n";
  boost::shared_ptr<SenderCallback> callback(new SenderCallback(maxOutstanding));
  callback->outCnt_ = 0;
  do {
    // Make async scheduling decisions here.
    DbosStatus status;
    if (broadcast) {
      // Broadcasts to all partitions of the database.
      status = scheduler->asyncBroadcastMessage(callback);
    } else {
      status = scheduler->asyncSendMessage(0, partitions, callback);
    }
    assert(status);
    callback->outCnt_++;
    std::this_thread::sleep_for(std::chrono::milliseconds(arrivalDelay));
    if (callback->outCnt_ >= maxOutstanding) {
      // Run the event loop once; return immediately if no responses. It will
      // process callbacks until the event loop finished or callback returns
      // true.
      // TODO: either find a better heuristic, or run callbacks on a separate thread.
      voltdbClient.runOnce();
      continue;
    }

    if (callback->outCnt_ > maxOutstanding/2) {
      // Heuristic to process responses in time.
      // TODO: either find a better heuristic, or run callbacks on a separate thread.
      voltdbClient.runOnce();
      continue;
    }


  } while (!mainFinished);

  // Give outstanding requests time to finish.
  while (!voltdbClient.drain()) {}

  // Clean up
  delete scheduler;
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
  msgLatencies = new double[kMaxEntries];
  memset(msgLatencies, 0, kMaxEntries * sizeof(double));
  msgLatsArrayIndex.store(0);
  msgIndices.push_back(0);

  // Start scheduler threads.
  for (int i = 0; i < numSenders; ++i) {
    schedulerThreads.push_back(
        new std::thread(&SenderThread, i, serverAddr));
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
    schedulerThreads[i]->join();
    delete schedulerThreads[i];
  }

  // Processing the results.
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      msgLatencies, msgIndices, timeStampsUsec, outputFile, scheduleAlgo);
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

  // Clean up.
  delete[] msgLatencies;
  msgLatencies = nullptr;
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
  std::cerr << "\t-b: use DB for broadcast.\n";
  std::cerr << "\t-o <output log file path>: default "
            << "message_results.csv\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel senders (threads)>: default "
            << numSenders << "\n";
  std::cerr << "\t-P <partitions>: default " << partitions << "\n";
  std::cerr << "\t-d <arrival delay>: default " << arrivalDelay << "\n";
  // Max outstanding requests: if reaches this threshold, runs the event loop
  // once to process potential responses.
  std::cerr << "\t-m <max outstanding requests>: default " << maxOutstanding << "\n";
  // Print all options here.

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("message_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hxXbo:s:i:t:N:P:d:m:")) != -1) {
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
        numSenders = atoi(optarg);
        break;
      case 'P':
        partitions = atoi(optarg);
        break;
      case 'd':
        arrivalDelay = atoi(optarg);
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
      case 'b':
	broadcast = true;
	break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel sender threads: " << numSenders << std::endl;
  std::cerr << "Partitions: " << partitions << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";
  std::cerr << "Arrival delay: " << arrivalDelay << " msec\n";
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
