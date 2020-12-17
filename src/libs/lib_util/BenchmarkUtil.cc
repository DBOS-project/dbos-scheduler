#include "BenchmarkUtil.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

bool BenchmarkUtil::processResults(double* latencies,
                                   const std::vector<uint32_t>& indices,
                                   const std::vector<uint64_t>& timeStampsUsec,
                                   const std::string& outputFile,
                                   const std::string& expName) {
  std::ofstream logFile(outputFile);
  if (!logFile.is_open()) {
    std::cerr << "Could not open file: " << outputFile << std::endl;
    return false;
  }
  logFile << "Title,TimeInUSecSinceEpoch,DurationInMsec,Count,Avg(usec),Stddev,"
             "Min,Max,P50,P90,P95,P99,Throughput\n";

  // Process one interval at a time.
  char outbuff[1024];
  size_t totalIntervals = indices.size();
  for (size_t i = 1; i < totalIntervals; ++i) {
    double durationMsec =
        (double)(timeStampsUsec[i] - timeStampsUsec[i - 1]) / 1000.0;
    size_t numEntries = indices[i] - indices[i - 1];
    // Note: this step can change the order within latencies array.
    auto stats =
        BenchmarkUtil::computeStats(latencies + indices[i - 1], numEntries);

    double throughput = numEntries * 1000.0 / durationMsec;
    BenchmarkUtil::printStats(stats, expName, throughput);

    // Print from "Count" to the end.
    sprintf(outbuff,
            "%lu,%.3lf,%.3lf,%.0lf,%.0lf,%.0lf,%.0lf,%.0lf,%.0lf,%.3lf\n",
            numEntries, stats.average, stats.stddev, stats.min, stats.max,
            stats.P50, stats.P90, stats.P95, stats.P99, throughput);

    if (i == 1) {
      // Duplicate the first stats, set duration to 0. Easier for plotting.
      logFile << expName << "," << timeStampsUsec[0] << ",0.0," << outbuff;
    }
    logFile << expName << "," << timeStampsUsec[i] << "," << durationMsec << ","
            << outbuff;
  }

  // Print aggregated throughput.
  uint32_t totalEntries = indices[totalIntervals - 1];
  double totalTimeSec =
      (timeStampsUsec[totalIntervals - 1] - timeStampsUsec[0]) / 1000000.0;
  std::cout << "Total transactions: " << totalEntries << "\n";
  std::cout << "Average throughput: " << (double)totalEntries / totalTimeSec
            << "\n";

  std::cerr << "Stored results to: " << outputFile << std::endl;
  logFile.close();
  return true;
}

// Borrow the idea from https://github.com/PlatformLab/PerfUilts/
//        PerfUtils/src/Stats.cc
BenchmarkUtil::Statistics BenchmarkUtil::computeStats(double* rawData,
                                                      size_t count) {
  BenchmarkUtil::Statistics stats;
  memset(&stats, 0, sizeof(stats));
  if (count == 0) { return stats; }

  // Sort rawData array. Thus, we are changing rawData content.
  std::sort(rawData, rawData + count);

  // Calculate total latency
  double sum = 0;
  for (size_t i = 0; i < count; ++i) { sum += rawData[i]; }
  stats.count = count;
  stats.average = sum / count;
  stats.stddev = 0;

  // Calculate stddev
  for (size_t i = 0; i < count; ++i) {
    double diff = std::fabs(rawData[i] - stats.average);
    stats.stddev += diff * diff;
  }
  stats.stddev /= count;
  stats.stddev = std::sqrt(stats.stddev);

  stats.min = rawData[0];
  stats.max = rawData[count - 1];
  stats.P50 = rawData[count / 2];
  stats.P90 = rawData[static_cast<int>(count * 0.9)];
  stats.P95 = rawData[static_cast<int>(count * 0.95)];
  stats.P99 = rawData[static_cast<int>(count * 0.99)];
  return stats;
}

void BenchmarkUtil::printStats(const BenchmarkUtil::Statistics& stats,
                               const std::string& header,
                               const double throughput) {
  static bool headerPrinted = false;
  if (!headerPrinted) {
    printf(
        "Bench,\tCount,\tAvg,\tStddev,\tMedian,\tMin,\tMax,\tP99,"
        "\tThroughput\n");
    headerPrinted = true;
  }
  printf(
      "%s,\t%lu,\t%7.1lf,\t%7.1lf,\t%7.1lf,\t%7.1lf,\t%7.1lf,\t%7.1lf,\t%7."
      "1lf\n",
      header.c_str(), stats.count, stats.average, stats.stddev, stats.P50,
      stats.min, stats.max, stats.P99, throughput);
  fflush(stdout);
}
