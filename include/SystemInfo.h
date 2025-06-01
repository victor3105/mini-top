#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <stdint.h>

#include <fstream>
#include <vector>

// Describes RAM usage
struct MemoryUsage {
  long totalKB;
  long availableKB;
  long usedKB;
  double usedPercent;
};

struct CpuUsage {
  double totalUsage;
  std::vector<double> perCoreUsage;
};

// CPU activity information
struct CpuTimes {
  // Describes total amount of time spent by CPU
  long total;
  // Amount of time when CPU was idle
  long idle;
};

// Collect system metrics (CPU/RAM usage)
class SystemInfo {
 public:
  CpuUsage getCpuUsage() const;
  MemoryUsage getMemoryUsage() const;
  CpuTimes getCpuTimes(std::string& str) const;

 private:
  void collectPerCoreSnapshots(std::ifstream& statFile, unsigned numCores,
                               std::vector<CpuTimes>& snapshots) const;
};

#endif /* SYSTEMINFO_H */
