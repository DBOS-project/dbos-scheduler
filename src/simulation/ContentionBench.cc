// This **asynchronous** communication benchmark will create
// "fake" messages and insert them to the DB. There won't be actual receivers.
// The goal is to benchmark the basic latency, throughput, and scalability of
// different communication methods.

#include <getopt.h>
#include <atomic>
#include <boost/shared_ptr.hpp>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

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

// Number of receivers.
static int numReceivers = 1;

// Number of spinners.
static int numSpinners = 1;
// Number of microseconds to spin.
static long spinUS = 100;

// Report throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static std::vector<uint64_t> receiver_txs;
static std::vector<uint64_t> spinner_txs;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

// Barrier used for thread synchronization.
pthread_barrier_t barrier;

void spin_tx(voltdb::Client* client, const long spin_us) {
  std::vector<voltdb::Parameter> parameterTypes(2);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_BIGINT);
  voltdb::Procedure procedure("Spin", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  // Hardcode pkey to 0.
  params->addInt32(0).addInt64(spin_us);
  voltdb::InvocationResponse r = client->invoke(procedure);
  if (r.failure()) {
    std::cerr << "Spin procedure failed. " << r.toString();
    exit(-1);
  }
}

static void ReceiverThread(const int receiverId, const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  client.createConnection(serverAddr);

  std::cout << "Receiver: " << receiverId << " started\n";
  // Wait until all senders and receivers have connected.
  pthread_barrier_wait(&barrier);
  do {
    // Receivers return immediately.
    spin_tx(&client, 0);
    receiver_txs[receiverId]++;
  } while (!mainFinished);

  return;
}

static void SpinnerThread(const int spinnerId, const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  client.createConnection(serverAddr);

  std::cout << "Spinner: " << spinnerId << " started\n";
  // Wait until all senders and receivers have connected.
  pthread_barrier_wait(&barrier);
  do {
    // Receivers return immediately.
    spin_tx(&client, spinUS);
    spinner_txs[spinnerId]++;
  } while (!mainFinished);

  return;
}

void print_stats() {
  std::cout << "Receiver TXs" << std::endl;
  for (int i = 0; i < numReceivers; ++i) {
    std::cout << receiver_txs[i] << " ";
  }
  std::cout << std::endl << std::endl;

  std::cout << "Spinner TXs" << std::endl;
  for (int i = 0; i < numSpinners; ++i) {
    std::cout << spinner_txs[i] << " ";
  }
  std::cout << std::endl << std::endl;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr) {
  mainFinished = false;

  std::vector<std::thread*> receiverThreads;  // Threads doing a no-op in the DB.
  std::vector<std::thread*> spinnerThreads; // Threads busy spinning in the DB.

  // Start threads.
  receiver_txs.resize(numReceivers, 0);
  for (int i = 0; i < numReceivers; ++i) {
    receiverThreads.push_back(new std::thread(&ReceiverThread, i, serverAddr));
  }

  spinner_txs.resize(numSpinners, 0);
  for (int i = 0; i < numSpinners; ++i) {
    spinnerThreads.push_back(new std::thread(&SpinnerThread, i, serverAddr));
  }

  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    currTime = BenchmarkUtil::getCurrTimeUsec();
    std::cerr << "runBenchmark performance...\n";
    print_stats();
    // TODO: Print stats.
  } while (currTime < endTime);

  mainFinished = true;
  for (int i = 0; i < numReceivers; ++i) {
    receiverThreads[i]->join();
    delete receiverThreads[i];
  }
  for (int i = 0; i < numSpinners; ++i) {
    spinnerThreads[i]->join();
    delete spinnerThreads[i];
  }

  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-R <number of parallel receivers (threads)>: default "
            << numReceivers << "\n";
  std::cerr << "\t-S <number of parallel spinners (threads)>: default "
            << numSpinners << "\n";
  std::cerr << "\t-b <busy-spinning interval>: default " << spinUS << "\n";
  // Print all options here.

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "h:s:i:t:R:S:b:")) != -1) {
    switch (opt) {
      case 's':
        serverAddr = optarg;
        break;
      case 'i':
        measureIntervalMsec = atoi(optarg);
        break;
      case 't':
        totalExecTimeMsec = atoi(optarg);
        break;
      case 'R':
        numReceivers = atoi(optarg);
        break;
      case 'S':
        numSpinners = atoi(optarg);
        break;
      case 'b':
        spinUS = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel receiver threads: " << numReceivers << std::endl;
  std::cerr << "Parallel spinner threads: " << numSpinners << std::endl;
  std::cerr << "Spinning interval: " << spinUS << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  // Initialize barrier.
  pthread_barrier_init(&barrier, NULL, numReceivers + numSpinners);

  // Run experiments.
  auto res = runBenchmark(serverAddr);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}
