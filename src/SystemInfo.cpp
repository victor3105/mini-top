#include "SystemInfo.h"

#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

CpuTimes SystemInfo::getCpuTimes(std::string& str) const {
  unsigned long user, nice, system, idle, iowait, irq, softirq;
  long totalTime, idleTime;
  std::string label;

  std::istringstream iss(str);
  iss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
  totalTime = user + nice + system + idle + iowait + irq + softirq;
  idleTime = idle + iowait;

  return (CpuTimes){.total = totalTime, .idle = idleTime};
}

static int getLogicalCoreCount() {
  long count = sysconf(_SC_NPROCESSORS_ONLN);

  return (count > 0) ? static_cast<int>(count) : 1;
}

void SystemInfo::collectPerCoreSnapshots(
    std::ifstream& statFile, unsigned numCores,
    std::vector<CpuTimes>& snapshots) const {
  std::string cpuStr;
  CpuTimes snapshotTmp;

  for (int i = 0; i < numCores; i++) {
    getline(statFile, cpuStr);
    snapshotTmp = getCpuTimes(cpuStr);
    snapshots.push_back(snapshotTmp);
  }
}

double calcUsage(double totalDelta, double idleDelta) {
  if (totalDelta == 0) {
    return 0.0;
  }

  return ((totalDelta - idleDelta) / totalDelta) * 100.0;
}

CpuUsage SystemInfo::getCpuUsage() const {
  // Sample the current CPU times
  std::string cpuStr;
  CpuTimes snapshot1, snapshot2;
  std::vector<CpuTimes> perCoreSnapshot1;
  std::vector<CpuTimes> perCoreSnapshot2;
  int numCores = getLogicalCoreCount();
  CpuUsage stats;

  // Read first snapshot from /proc/stat
  std::ifstream statFile("/proc/stat");
  getline(statFile, cpuStr);
  snapshot1 = getCpuTimes(cpuStr);

  collectPerCoreSnapshots(statFile, numCores, perCoreSnapshot1);

  // Sleep for a short period to get the second snapshot
  std::this_thread::sleep_for(
      std::chrono::milliseconds(this->snapshotsSleepMs));

  statFile.clear();
  statFile.seekg(0);

  // Read second snapshot from /proc/stat
  getline(statFile, cpuStr);
  snapshot2 = getCpuTimes(cpuStr);

  collectPerCoreSnapshots(statFile, numCores, perCoreSnapshot2);

  statFile.close();

  // Calculate CPU usage percentage
  double totalDelta = snapshot2.total - snapshot1.total;
  double idleDelta = snapshot2.idle - snapshot1.idle;

  stats.totalUsage = calcUsage(totalDelta, idleDelta);

  for (int i = 0; i < numCores; i++) {
    double perCoreTotalDelta =
        perCoreSnapshot2[i].total - perCoreSnapshot1[i].total;
    double perCoreIdleDelta =
        perCoreSnapshot2[i].idle - perCoreSnapshot1[i].idle;
    double perCorePercent =
        calcUsage(perCoreTotalDelta, perCoreIdleDelta);

    stats.perCoreUsage.push_back(perCorePercent);
  }

  return stats;
}

MemoryUsage SystemInfo::getMemoryUsage() const {
  std::ifstream statFile("/proc/meminfo");
  std::string memStr;
  std::vector<long> values;
  long val;
  std::string label;
  std::string units;
  MemoryUsage result;

  for (int i = 0; i < 3; i++) {
    getline(statFile, memStr);

    std::istringstream iss(memStr);

    iss >> label >> val >> units;

    values.push_back(val);
  }

  result.totalKB = values[0];
  // We use simplified logic here, we might need something like
  // MemUsed = MemTotal - MemFree - Buffers - Cached - SReclaimable + Shmem
  result.usedKB = result.totalKB - values[1];
  result.availableKB = values[2];
  result.usedPercent = (double)result.usedKB / (double)result.availableKB * 100;

  return result;
}
