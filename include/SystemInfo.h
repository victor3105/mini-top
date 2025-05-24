#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <stdint.h>
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

// Collect system metrics (CPU/RAM usage)
class SystemInfo {
  public:
    CpuUsage getCpuUsage() const;
    MemoryUsage getMemoryUsage() const;
};

#endif /* SYSTEMINFO_H */
