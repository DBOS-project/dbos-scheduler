// Server for basic DBOS IPC benchmarks.

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

// Number of receivers.
static int numReceivers = 1;

// Number of outstanding messages.
static int numMessages = 1;

// Message size.
static int msg_size = 2048;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

// Barrier used for thread synchronization.
pthread_barrier_t barrier;

/*
 * Poll VoltDB for RX messages.
 * If at least one message is found, store the sender_id.
 * Assumes that all messages arrived from a single sender.
 */
int dbos_recv(voltdb::Client* client, int* sender_id, const int receiverID) {
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
  voltdb::TableIterator resIter = results.iterator();
  if (resIter.hasNext()) {
    voltdb::Row row = resIter.next();
    *sender_id = row.getInt32(0);
  }
  return results.rowCount();
}

/*
 * A callback that counts the number of times that it is invoked and returns true
 * when the counter reaches zero to instruct the client library to break out of the event loop.
 */
class SendCallback : public voltdb::ProcedureCallback {
  public:
    SendCallback(int count) : m_count(count), o_count(count) {}

    bool callback(voltdb::InvocationResponse response) throw (voltdb::Exception) {
      m_count--;

      //Print the error response if there was a problem
      if (response.failure()) {
        std::cout << response.toString();
      }

      //If the callback has been invoked count times, return true to break event loop
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
void dbos_send(voltdb::Client* client, boost::shared_ptr<voltdb::ProcedureCallback> callback,
               const int receiver_id, const int sender_id, const std::string &data) {
  std::vector<voltdb::Parameter> parameterTypes(4);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_BIGINT);
  parameterTypes[3] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
  voltdb::Procedure procedure("SendMessage", parameterTypes);

  for (int i = 0; i < numMessages; ++i) {
    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(receiver_id).addInt32(sender_id);
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
 * Sender thread.
 * 
 */
static void ReceiverThread(const int threadId,
                         const std::string& serverAddr) {
  // Id of the client.
  int clientId = -1;

  // Create a local VoltDB client.
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  client.createConnection(serverAddr);

  // Create the message.
  std::string data(msg_size, '0');

  // Initialize callback.
  boost::shared_ptr<SendCallback> callback(new SendCallback(numMessages));

  int recvd = 0;
  while (recvd < numMessages) {
    recvd += dbos_recv(&client, &clientId, threadId);
  }
  dbos_send(&client, callback, clientId, threadId, data);

  // Wait until all senders and receivers have connected.
  pthread_barrier_wait(&barrier);

  while(1) {
    recvd = 0;
    while (recvd < numMessages) {
      recvd += dbos_recv(&client, &clientId, threadId);
    }
    dbos_send(&client, callback, clientId, threadId, data);
  }

  return;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  //TODO: support list of servers.
  std::cerr << "\t-s <DB server>: default 'localhost'\n";
  std::cerr << "\t-N <number of parallel receivers (threads)>: default "
            << numReceivers << "\n";
  std::cerr << "\t-M <number of parallel messages expected by each receiver>: "
            << "default " << numMessages << "\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "h:s:N:M:m:")) != -1) {
    switch (opt) {
      case 's':
        serverAddr = optarg;
        break;
      case 'N':
        numReceivers = atoi(optarg);
        break;
      case 'M':
        numMessages = atoi(optarg);
        break;
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel receiver threads: " << numReceivers << std::endl;
  std::cerr << "Parallel messages: " << numMessages << std::endl;
  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;

  std::vector<std::thread*> receiverThreads;  // Parallel receivers.

  // Initialize barrier.
  pthread_barrier_init(&barrier, NULL, numReceivers);

  // Start recceiver threads.
  for (int i = 0; i < numReceivers; ++i) {
    // Assumes that receivers have receiverId = senderId + 100.
    receiverThreads.push_back(
        new std::thread(&ReceiverThread, i + 100, serverAddr));
  }

  // We should never reach this point. 
  for (int i = 0; i < numReceivers; ++i) {
    receiverThreads[i]->join();
    delete receiverThreads[i];
  }

  return 0;
}
