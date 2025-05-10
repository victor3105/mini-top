#include <stdint.h>

/** Describes RAM usage */
struct MemoryUsage {
    long totalKB;
    long availableKB;
    long usedKB;
    double usedPercent;
};

/** Collect system metrics (CPU/RAM usage) */
class SystemInfo {
  public:
    double getCpuUsage() const;
    MemoryUsage getMemoryUsage() const;
};
