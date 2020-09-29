#include <algorithm>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include "BenchmarkUtil.h"

// Borrow the idea from https://github.com/PlatformLab/PerfUilts/
//        PerfUtils/src/Stats.cc
BenchmarkUtil::Statistics BenchmarkUtil::computeStats(double* rawData, size_t count) {
  BenchmarkUtil::Statistics stats;
  memset(&stats, 0, sizeof(stats));
  if (count == 0) {
    return stats;
  }

  // Sort rawData array. Thus, we are changing rawData content.
  std::sort(rawData, rawData + count);

  // Calculate total latency
  double sum = 0;
  for (size_t i = 0; i < count; ++i) {
    sum += rawData[i];
  }
  stats.count = count;
  stats.average = sum / count;
  stats.stddev = 0;
  
  // Calculate stddev
  for (size_t i= 0; i < count; ++i) {
    double diff = std::fabs(rawData[i] - stats.average);
    stats.stddev += diff * diff;
  }
  stats.stddev /= count;
  stats.stddev = std::sqrt(stats.stddev);
  
  stats.min = rawData[0];
  stats.max = rawData[count - 1];
  stats.P50 = rawData[count / 2];
  stats.P90 =
      rawData[static_cast<int>(count * 0.9)];
  stats.P95 =
      rawData[static_cast<int>(count * 0.95)];
  stats.P99 =
      rawData[static_cast<int>(count * 0.99)];
  return stats;
}

void BenchmarkUtil::printStats(
    const BenchmarkUtil::Statistics& stats, const std::string& header) {
  static bool headerPrinted = false;
  if (!headerPrinted) {
    printf("Benchmark,\tCount,\tAvg,\tStddev,\tMedian,\tMin,\tMax,\tP99\n");
    headerPrinted = true;
  }
  printf("%s,\t%lu,\t%lf,\t%lf,\t%lf,\t%lf,\t%lf,\t%lf\n", header.c_str(),
         stats.count, stats.average, stats.stddev, stats.P50, stats.min,
         stats.max, stats.P99);
  fflush(stdout);
}
