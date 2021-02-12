// This is a basic ping-pong TCP client.

#include <getopt.h>
#include <pthread.h>
#include <atomic>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <unordered_set>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "BenchmarkUtil.h"

// Number of senders.
static int numSenders = 1;

// Number of broadcast receivers.
static int numReceivers = 1;

// Base receiver port.
static int basePort = 8080;

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

// Broadcast flag.
static bool broadcast = false;

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static double* msgLatencies;
// The current index of the msgLatencies array.
static std::atomic<uint32_t> msgLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> msgIndices;
// Timestamp of each interval starts, in usec.
static std::vector<uint64_t> timeStampsUsec;

// Barrier used for thread synchronization.
pthread_barrier_t barrier;


/*
 * Setup connections.
 */
int setup_connections(std::vector<int>& socks, const int serverPort,
                      const std::string& serverAddr) {
  // Create epoll fd.
  int epoll_fd = epoll_create1(0);
  if(epoll_fd == -1) {
    std::cerr << "Failed to create epoll file descriptor\n";
    exit(-1);
  }

  // Connect to each receiver.
  for (int i = 0; i < numReceivers; ++i) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    if (sock < 0) {
      std::cerr << "Socket creation error" << std::endl;
      exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort + i);
    if (inet_pton(AF_INET, serverAddr.c_str(), &serv_addr.sin_addr) <= 0) {
      std::cerr << "Invalid address/ Address not supported" << std::endl;
      exit(-1);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cerr << "Connection failed" << std::endl;
      exit(-1);
    }

    // Save socket for later on.
    socks.push_back(sock);

    // Add connection to epoll.
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event)) {
      std::cerr << "Failed to add file descriptor to epoll\n";
      exit(-1);
    }
  }

  return epoll_fd;
}


static void BroadcasterThread(const int serverPort, const std::string& serverAddr) {
  std::vector<int> socks;
  struct epoll_event event, events[numReceivers];

  char buffer[msg_size] = {0};

  int epoll_fd = setup_connections(socks, serverPort, serverAddr);

  do {
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();

    // Send to all receivers.
    for (int i = 0; i < numReceivers; ++i) {
      int total_sent = 0;
      while (total_sent < msg_size) {
        int sent = write(socks[i], buffer + total_sent, msg_size - total_sent);
        if (sent < 0) {
          std::cerr << "Send failed" << std::endl;
          exit(-1);
        }
        total_sent += sent;
      }
    }

    // Read reply from all receivers.
    int replied = 0;
    while (replied < numReceivers) {
      int event_count = epoll_wait(epoll_fd, events, numReceivers, 600000);
      for(int i = 0; i < event_count; i++) {
	int total_read = 0;
        while (total_read < msg_size) {
          int recvd = read(events[i].data.fd, buffer + total_read, msg_size - total_read);
          if (recvd < 0) {
            std::cerr << "Receive failed" << std::endl;
            exit(-1);
          }
          total_read += recvd;
	}
	++replied;
      }
    }

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
 * Sender thread.
 */
static void SenderThread(const int serverPort, const std::string& serverAddr) {
  int ret, sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[msg_size] = {0};

  // Connect to the server.
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    exit(-1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(serverPort);
  if (inet_pton(AF_INET, serverAddr.c_str(), &serv_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    exit(-1);
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Connection failed" << std::endl;
    exit(-1);
  }

  // Wait until all senders have connected.
  pthread_barrier_wait(&barrier);

  // Run a ping-pong loop.
  do {
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();
    int total_sent = 0;
    while (total_sent < msg_size) {
      int sent = write(sock, buffer + total_sent, msg_size - total_sent);
      if (sent < 0) {
        std::cerr << "Send failed" << std::endl;
        exit(-1);
      }
      total_sent += sent;
    }

    int total_read = 0;
    while (total_read < msg_size) {
      int recvd = read(sock, buffer + total_read, msg_size - total_read);
      if (recvd < 0) {
        std::cerr << "Receive failed" << std::endl;
        exit(-1);
      }
      total_read += recvd;
    }
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

  // Start sender threads.
  if (broadcast) {
    for (int i = 0; i < numSenders; ++i) {
      senderThreads.push_back(new std::thread(&BroadcasterThread, basePort + i,
                                              serverAddr));
    }
  } else {
    for (int i = 0; i < numSenders; ++i) {
      senderThreads.push_back(new std::thread(&SenderThread, basePort + i,
                                              serverAddr));
    }
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
      msgLatencies, msgIndices, timeStampsUsec, outputFile, "TCP/IP ping-pong");
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

  // Clean up.
  delete[] msgLatencies;
  msgLatencies = nullptr;
  timeStampsUsec.clear();
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-b: run broadcast benchmark\n";
  std::cerr << "\t-o <output log file path>: default "
            << "message_results.csv\n";
  std::cerr << "\t-s <comma-separated list of servers>: default 'localhost'\n";
  std::cerr << "\t-p <port number>: default " << basePort << std::endl;
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-N <number of parallel senders (threads)>: default "
            << numSenders << "\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << "\t-R <number of receivers> used only when broadcasting: default "
            << numReceivers << "\n";
  // Print all options here.

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("127.0.0.1");
  std::string outputFile("message_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hbo:s:p:i:t:N:m:R:")) != -1) {
    switch (opt) {
      case 'o':
        outputFile = optarg;
        break;
      case 's':
        serverAddr = optarg;
        break;
      case 'p':
        basePort = atoi(optarg);
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
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'b':
	broadcast = true;
	break;
      case 'R':
        numReceivers = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel sender threads: " << numSenders << std::endl;
  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "Output log file: " << outputFile << std::endl;
  std::cerr << "Server address: " << serverAddr << std::endl;
  std::cerr << "Base port: " << basePort << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";
  if (broadcast) {
    std::cerr << "Broadcasting to " << numReceivers << " receivers\n";
  }

  // Run experiments and parse results.
  bool res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}
