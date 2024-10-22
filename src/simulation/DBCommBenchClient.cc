// Client for basic DBOS IPC benchmarks.

#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "BenchmarkUtil.h"
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/ProcedureCallback.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

// Number of senders.
static int numSenders = 1;

// Number of outstanding messages.
static int numMessages = 1;

// Message size.
static int msg_size = 2048;

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
static double* msgLatencies;
// The current index of the msgLatencies array.
static std::atomic<uint32_t> msgLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> msgIndices;
// Timestamp of each interval starts, in userc.
static std::vector<uint64_t> timeStampsUsec;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

// If true, truncate tables after execution.
static bool cleanDB = false;

// If true, run broadcasting benchmark.
static bool broadcast = false;

// Barrier used for thread synchronization.
pthread_barrier_t barrier;

/*
 * A callback that counts the number of times that it is invoked and returns
 * true
 * when the counter reaches zero to instruct the client library to break out of
 * the event loop.
 */
class SendCallback : public voltdb::ProcedureCallback {
public:
  SendCallback(int count) : m_count(count), o_count(count) {}

  bool callback(voltdb::InvocationResponse response) throw(voltdb::Exception) {
    m_count--;

    // Print the error response if there was a problem
    if (response.failure()) { std::cout << response.toString(); }

    // If the callback has been invoked count times, return true to break event
    // loop
    if (m_count == 0) {
      m_count = o_count;
      return true;
    } else {
      return false;
    }
  }

private:
  int m_count;
  int o_count;
};

/*
 * Send messages using VoltDB.
 *
 */
void dbos_send(voltdb::Client* client,
               boost::shared_ptr<voltdb::ProcedureCallback> callback,
               const int receiver_id, const int sender_id,
               const std::string& data) {
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_BIGINT);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
  voltdb::Procedure procedure("SendMessage", parameterTypes);

  int receiver = receiver_id;

  for (int i = 0; i < numMessages; ++i) {
    voltdb::ParameterSet* params = procedure.params();
    if (broadcast) { receiver = receiver_id + i; }
    params->addInt32(receiver).addInt32(sender_id);
    params->addInt64(BenchmarkUtil::getCurrTimeUsec()).addString(data);
    client->invoke(procedure, callback);
  }

  /*
   * Run the client event loop to poll the network and invoke callbacks.
   * The event loop will break on an error or when a callback returns true
   */
  client->run();
}

/*
 * Poll VoltDB for RX messages.
 *
 */
int dbos_recv(voltdb::Client* client, const int receiverID) {
  std::vector<voltdb::Parameter> parameterTypes(1);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);

  voltdb::Procedure procedure("ReceiveMessage", parameterTypes);
  voltdb::ParameterSet* params = procedure.params();
  params->addInt32(receiverID);
  voltdb::InvocationResponse r = client->invoke(procedure);
  if (r.failure()) {
    std::cerr << "ReceiveMessage procedure failed. " << r.toString();
    exit(-1);
  }

  voltdb::Table results = r.results()[0];
  return results.rowCount();
}

/*
 * Sender thread.
 *
 */
static void SenderThread(const int threadId, const std::string& serverAddr) {
  // Create a local VoltDB client.
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  client.createConnection(serverAddr);

  // Create the message.
  std::string data(msg_size, '0');

  // Initialize callback.
  boost::shared_ptr<SendCallback> callback(new SendCallback(numMessages));

  // Assume that receivers have receiverId = threadId + 100.
  dbos_send(&client, callback, threadId + 100, threadId, data);
  int recvd = 0;
  while (recvd < numMessages) { recvd += dbos_recv(&client, threadId); }

  // Wait until all senders and receivers have connected.
  pthread_barrier_wait(&barrier);

  do {
    recvd = 0;
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();
    dbos_send(&client, callback, threadId + 100, threadId, data);
    while (recvd < numMessages) { recvd += dbos_recv(&client, threadId); }
    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();

    // Record the latency.
    auto aryIndex = msgLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array msgLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    msgLatencies[aryIndex] = double(endTime - startTime);
  } while (!mainFinished);

  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::vector<std::thread*> senderThreads;  // Parallel senders.

  // Initialize measurement arrays.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  timeStampsUsec.push_back(currTime);
  msgLatencies = new double[kMaxEntries];
  memset(msgLatencies, 0, kMaxEntries * sizeof(double));
  msgLatsArrayIndex.store(0);
  msgIndices.push_back(0);

  // Initialize barrier.
  pthread_barrier_init(&barrier, NULL, numSenders);

  for (int i = 0; i < numSenders; ++i) {
    senderThreads.push_back(new std::thread(&SenderThread, i, serverAddr));
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
    senderThreads[i]->join();
    delete senderThreads[i];
  }

  // Processing the results.
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      msgLatencies, msgIndices, timeStampsUsec, outputFile, "DBOS Ping Pong");
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
 * Cleanup database.
 */
static bool teardown(const std::string& serverAddr) {
  // Create a local VoltDB client.
  // voltdb::Client voltdbClient =
  //    VoltdbSchedulerUtil::createVoltdbClient(kTestUser, kTestPwd);
  // voltdbClient.createConnection(serverAddr);
  // Fill me.
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-x: truncate DB tables after execution.\n";
  std::cerr << "\t-b: send each parallel message to different receiver.\n";
  std::cerr << "\t-o <output log file path>: default "
            << "synthetic_scheduler_results.csv\n";
  // TODO: support list of servers.
  std::cerr << "\t-s <DB server>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel senders (threads)>: default "
            << numSenders << "\n";
  std::cerr << "\t-M <number of parallel messages sent by each sender>: "
            << "default " << numMessages << "\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("dbos_ipc_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hxbo:s:i:t:N:M:m:")) != -1) {
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
      case 'M':
        numMessages = atoi(optarg);
        break;
      case 'N':
        numSenders = atoi(optarg);
        break;
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'x':
        cleanDB = true;
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
  std::cerr << "Parallel messages: " << numMessages << std::endl;
  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";
  if (broadcast) { std::cerr << "Broadcasting\n"; }

  // 1) Run experiments and parse results.
  bool res;
  res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  // 2) Clean database state.
  if (cleanDB) {
    res = teardown(serverAddr);
    if (!res) {
      std::cerr << "Failed to tear down database state." << std::endl;
      exit(1);
    }
  }

  return 0;
}
