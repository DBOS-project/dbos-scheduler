// This synthetic multi-thread scheduler will create "fake" task ids, and
// queries the DB to find a worker.  There won't be actual worker or load
// generator.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel schedulers.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "simulation/BenchmarkUtil.h"
#include "simulation/GRPCSparkScheduler.h"
#include "simulation/PartitionedFIFOScheduler.h"
#include "simulation/PartitionedFIFOTaskScheduler.h"
#include "simulation/PartitionedScanTask.h"
#include "simulation/PushFIFOScheduler.h"
#include "simulation/SinglePartitionedFIFOTaskScheduler.h"
#include "simulation/SparkScheduler.h"
#include "simulation/VoltdbSchedulerUtil.h"
#include "voltdb-client-cpp/include/Client.h"

#include <grpcpp/grpcpp.h>

#include "frontend.grpc.pb.h"

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

// Wait interval between requests.
static int arrivalDelay = 0;

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

// If true, truncate tables after execution.
static bool cleanDB = false;

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

  int port = 8000 + schedulerId;
  GRPCSparkScheduler* scheduler = new GRPCSparkScheduler(port);
  scheduler->setup();
  assert(scheduler != nullptr);
  std::cout << "Scheduler: " << schedulerId << " started\n";

  std::string addr = "localhost:" + std::to_string(port);

  std::shared_ptr<Channel> channel =
      grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<dbos_scheduler::Frontend::Stub> stub =
      dbos_scheduler::Frontend::NewStub(channel);

  do {
    auto aryIndex = schedLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array schedLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();

    // Submit task to scheduler.
    dbos_scheduler::SubmitTaskRequest st_request;
    st_request.set_requirement(10);
    st_request.set_exectime(1000);
    dbos_scheduler::SubmitTaskResponse st_reply;

    ClientContext st_context;

    Status status = stub->SubmitTask(&st_context, st_request, &st_reply);
    assert(status.ok());

    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();
    // Record latency.
    double latency = (double)(endTime - startTime);
    schedLatencies[aryIndex] = latency;
    std::this_thread::sleep_for(std::chrono::milliseconds(arrivalDelay));
  } while (!mainFinished);

  sleep(1); // Give outstanding requests time to finish.
  scheduler->teardown();
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
      schedLatencies, schedIndices, timeStampsUsec, outputFile, "Spark");
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

  VoltdbSchedulerUtil* scheduler = new SparkScheduler(&voltdbClient, serverAddr, partitions,
                                   workerCapacity, numWorkers);
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

  VoltdbSchedulerUtil* scheduler = new SparkScheduler(&voltdbClient, serverAddr, partitions,
                                   workerCapacity, numWorkers);
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
  std::cerr << "\t-d <arrival delay>: default " << partitions << "\n";
  std::cerr
      << "\t-p <probability of multi-partition transaction> (0-1.0): default "
      << probMultiTx << "\n";

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("synthetic_scheduler_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hxo:s:i:t:N:W:C:P:A:T:p:d:")) != -1) {
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
      case 'T':
        numTasks = atoi(optarg);
        break;
      case 'p':
        probMultiTx = atof(optarg);
        break;
      case 'd':
        arrivalDelay = atoi(optarg);
      case 'x':
        cleanDB = true;
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Scheduler algorithm: " << "Spark" << std::endl;
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
  std::cerr << "Arrival delay: " << arrivalDelay << " msec\n";

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