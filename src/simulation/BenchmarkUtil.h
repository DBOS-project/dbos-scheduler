// This file contains some common util functions for the benchmark.
#ifndef DBOS_BENCHMARK_UTIL_H
#define DBOS_BENCHMARK_UTIL_H

#include <cstdint>
#include <string>

class BenchmarkUtil {
public:
  // Latency stats in microseconds.
  struct Statistics {
    size_t count;
    double average;
    double stddev;
    double min;
    double max;
    double P50;
    double P90;
    double P95;
    double P99;
  };

  // Calculate stats from rawData array of latencies, consider `count` elements.
  static Statistics computeStats(double* rawData, size_t count);

  // Print the stats with a header.
  static void printStats(const Statistics& stats, const std::string& header);

  // Get current timestamp in usec.
  inline uint64_t getCurrTimeUsec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return timespecToUsec(ts);
  }

private:
  // Convert timespec to uint64_t timestamp in usec.
  inline uint64_t timespecToUsec(const struct timespec& a) {
    return (uint64_t)a.tv_sec * 1000000 + (uint64_t)a.tv_nsec / 1000;
  }

};

#endif  // #ifndef DBOS_BENCHMARK_UTIL_H
