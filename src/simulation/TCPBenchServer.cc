// This is a basic TCP ping-pong server.

#include <arpa/inet.h>
#include <getopt.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <boost/shared_ptr.hpp>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

// Number of receivers.
static int numReceivers = 1;

// Base receiver port.
static int basePort = 8080;

// Message size.
static int msg_size = 2048;

// Barrier used for thread synchronization.
pthread_barrier_t barrier;

/*
 * Receiver thread.
 */
static void ReceiverThread(const int serverPort) {
  int ret, server_fd = 0;
  int opt = 1;
  struct sockaddr_in addr;
  int addrlen = sizeof(addr);
  char buffer[msg_size] = {0};

  // Create server socket.
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    exit(-1);
  }

  // Set socket options.
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    std::cerr << "setsockopt error" << std::endl;
    exit(-1);
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(serverPort);

  if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "bind error" << std::endl;
    exit(-1);
  }

  if (listen(server_fd, 1) < 0) {
    std::cerr << "listen error" << std::endl;
    exit(-1);
  }

  int recv_fd =
      accept(server_fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
  if (recv_fd < 0) {
    std::cerr << "accept error" << std::endl;
    exit(-1);
  }

  // Wait until all senders have connected.
  pthread_barrier_wait(&barrier);

  while (true) {
    int total_read = 0;
    while (total_read < msg_size) {
      int recvd = read(recv_fd, buffer + total_read, msg_size - total_read);
      total_read += recvd;
    }
    int total_sent = 0;
    while (total_sent < msg_size) {
      int sent = write(recv_fd, buffer + total_sent, msg_size - total_sent);
      total_sent += sent;
    }
  }

  return;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << "\t-p <port number>: default " << basePort << std::endl;
  std::cerr << "\t-N <number of parallel receivers (threads)>: default "
            << numReceivers << "\n";
  // Print all options here.

  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "hm:p:N:")) != -1) {
    switch (opt) {
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'p':
        basePort = atoi(optarg);
        break;
      case 'N':
        numReceivers = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Parallel receiver threads: " << numReceivers << std::endl;
  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "Base port: " << basePort << std::endl;

  std::vector<std::thread*> receiverThreads;  // Parallel receivers.

  // Initialize barrier.
  pthread_barrier_init(&barrier, NULL, numReceivers);

  // Start scheduler threads.
  for (int i = 0; i < numReceivers; ++i) {
    receiverThreads.push_back(new std::thread(&ReceiverThread, basePort + i));
  }

  // Sleep forever.
  while (1) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }

  return 0;
}
