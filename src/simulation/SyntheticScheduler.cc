// This synthetic multi-thread scheduler will create "fake" task ids, and
// queries the DB to find a worker.  There won't be actual worker or load
// generator.
// The goal is to benchmark the basic latency, throughput, and scalability of
// parallel scheudlers.

#include <getopt.h>
#include <atomic>
#include <string>
#include <vector>

#include "BenchmarkUtil.h"
#include "VoltdbClientUtil.h"
#include "voltdb-client-cpp/include/Client.h"

// Number of schedulers and workers
static int numSchedulers = 1;
static int numWorkers = 1;

// Assume each worker has infinite capacity.
// TODO: make it tunable.
static int workerCapacity = (1L << 27);

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int ARRAY_EXP = 26;
static const size_t MAX_ENTRIES = (1L << ARRAY_EXP);

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

static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  mainFinished = true;
  return true;
}

static bool initWorkerTable() {
  voltdb::Client voltdbClient =
      VoltdbClientUtil::createVoltdbClient("fakeuser", "fakepwd");
  VoltdbClientUtil client(&voltdbClient, "localhost");

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

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("synthetic_scheduler_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "ho:s:i:t:N:W:")) != -1) {
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
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel scheduler threads: " << numSchedulers
            << "; workers: " << numWorkers << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  // 1) Initialize worker table in the database, add workers.
  bool res = initWorkerTable();
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
