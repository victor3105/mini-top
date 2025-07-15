#pragma once

#include <stdint.h>

#include <fstream>
#include <vector>

// Describes RAM usage
struct MemoryUsage {
  uint64_t totalKB;
  uint64_t availableKB;
  uint64_t usedKB;
  double usedPercent;
};

// Describes CPU usage
struct CpuUsage {
  double totalUsage;
  std::vector<double> perCoreUsage;
};

// CPU activity information
struct CpuTimes {
  // Describes total amount of time spent by CPU
  uint64_t total;
  // Amount of time when CPU was idle
  uint64_t idle;
};

// Collect system metrics (CPU/RAM usage)
class SystemInfo {
 public:
  // Get CPU usage info
  CpuUsage getCpuUsage() const;
  // Get RAM usage info
  MemoryUsage getMemoryUsage() const;
  // Get CPU times info (total/idle)
  CpuTimes getCpuTimes(std::string& str) const;
  SystemInfo(unsigned sleepMs = 100) : snapshotsSleepMs(sleepMs) {}

 private:
  // Time to sleep between consecutive snapshots in ms
  uint32_t snapshotsSleepMs;
  // Get CPU per-core usage info
  void collectPerCoreSnapshots(std::ifstream& statFile, unsigned numCores,
                               std::vector<CpuTimes>& snapshots) const;
};
