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
#include "simulation/SchedulerServer.h"
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
static int numClientThreads = 1;
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

// Type of scheduler algorithm
// TODO: add more types here.
static const std::string kFifoAlgo = "fifo";
static const std::string kFifoTaskAlgo = "fifo-task";
static const std::string kSinglePartitionedFifoTaskAlgo =
    "single-partitioned-fifo-task";
static const std::string kSparkAlgo = "spark";
static const std::string kScanTaskAlgo = "scan-task";
static const std::string kPushFifoAlgo = "push-fifo";
static const std::unordered_set<std::string> kAlgorithms = {
    kFifoAlgo, kFifoTaskAlgo, kSinglePartitionedFifoTaskAlgo, kSparkAlgo,
    kScanTaskAlgo, kPushFifoAlgo};
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
static void ClientThread(std::vector<std::string> schedulerAddresses) {

  std::vector<std::shared_ptr<Channel>> channels;
  for (std::string addr: schedulerAddresses) {
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    channels.push_back(channel);
  }

  do {
    auto aryIndex = schedLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array schedLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();

    // Submit task to scheduler.
    Task task;
    task.targetData = rand() % numWorkers;
    task.execTime = 1000;
    dbos_scheduler::SubmitTaskRequest st_request = taskToProtobuf(&task);
    dbos_scheduler::SubmitTaskResponse st_reply;

    ClientContext st_context;

    std::shared_ptr<Channel> channel = channels[rand() % channels.size()];
    std::unique_ptr<dbos_scheduler::Frontend::Stub> stub =
        dbos_scheduler::Frontend::NewStub(channel);
    Status status = stub->SubmitTask(&st_context, st_request, &st_reply);
    assert(status.ok());

    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();
    // Record latency.
    double latency = (double)(endTime - startTime);
    schedLatencies[aryIndex] = latency;
    std::this_thread::sleep_for(std::chrono::milliseconds(arrivalDelay));
  } while (!mainFinished);

  sleep(1); // Give outstanding requests time to finish.
  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::vector<std::thread*> clientThreads;  // Parallel client threads.

  // Initialize measurement arrays.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  timeStampsUsec.push_back(currTime);
  schedLatencies = new double[kMaxEntries];
  memset(schedLatencies, 0, kMaxEntries * sizeof(double));
  schedLatsArrayIndex.store(0);
  schedIndices.push_back(0);
  std::vector<SchedulerServer*> schedulers;
  std::vector<std::string> schedulerAddresses;
  std::vector<voltdb::Client> clients;

  for (int schedulerId = 0; schedulerId < numSchedulers; schedulerId++) {
    clients.push_back(VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd));
  }

  for (int schedulerId = 0; schedulerId < numSchedulers; schedulerId++) {
    int port = 9000 + schedulerId;

    VoltdbSchedulerUtil* scheduler =
        constructScheduler(&clients[schedulerId], serverAddr, "spark");
    SchedulerServer* schedulerServer =
        new SchedulerServer(port, scheduler);

    schedulers.push_back(schedulerServer);
    assert(scheduler != nullptr);
    std::cout << "Scheduler: " << schedulerId << " started\n";

    std::string addr = "localhost:" + std::to_string(port);
    schedulerAddresses.push_back(addr);
  }

  // Start scheduler threads.
  for (int i = 0; i < numClientThreads; ++i) {
    clientThreads.push_back(
        new std::thread(&ClientThread, schedulerAddresses));
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

  for (std::thread* schedulerThread: clientThreads) {
    schedulerThread->join();
    delete schedulerThread;
  }

  for (SchedulerServer* scheduler: schedulers) {
    delete scheduler;
  }

  // Processing the results.
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      schedLatencies, schedIndices, timeStampsUsec, outputFile, scheduleAlgo);
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

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
  std::cerr << "\t-N <number of parallel client threads>: default "
            << numClientThreads << "\n";
  std::cerr << "\t-S <number of schedulers>: default "
            << numSchedulers << "\n";
  std::cerr << "\t-W <number of workers (#rows in table)>: default "
            << numWorkers << "\n";
  std::cerr << "\t-C <worker capacity>: default " << workerCapacity << "\n";
  std::cerr << "\t-T <number of tasks>: default " << numTasks << "\n";
  std::cerr << "\t-P <partitions>: default " << partitions << "\n";
  std::cerr << "\t-d <arrival delay>: default " << arrivalDelay << "\n";
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
  while ((opt = getopt(argc, argv, "hxo:s:i:t:N:S:W:C:P:A:T:p:d:")) != -1) {
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
        numClientThreads = atoi(optarg);
        break;
      case 'S':
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

  // Check scheduler algorithm type valid here.
  auto algoIt = kAlgorithms.find(scheduleAlgo);
  if (algoIt == kAlgorithms.end()) {
    std::cerr << "Unsupported algorithm: " << scheduleAlgo << std::endl;
    Usage(argv);
  }
  std::cerr << "Scheduler algorithm: " << scheduleAlgo << std::endl;
  std::cerr << "Probability of multi-partition transaction: " << probMultiTx
            << std::endl;
  std::cerr << "Parallel scheduler threads: " << numClientThreads
            << "; workers: " << numWorkers << "; tasks: " << numTasks
            << "; schedulers: " << numSchedulers
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
