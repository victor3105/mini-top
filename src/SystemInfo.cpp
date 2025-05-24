#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "SystemInfo.h"

// CPU activity information
struct CpuTimes {
    // Describes total amount of time spent by CPU
    long total;
    // Amount of time when CPU was idle
    long idle;
};

static CpuTimes getCpuTimes(std::string& str)
{
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    long totalTime, idleTime;
    std::string label;

    std::istringstream iss(str);
    iss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
    totalTime = user + nice + system + idle + iowait + irq + softirq;
    idleTime = idle + iowait;

    return (CpuTimes){ .total = totalTime, .idle = idleTime };
}

static int getLogicalCoreCount() {
    long count = sysconf(_SC_NPROCESSORS_ONLN);

    return (count > 0) ? static_cast<int>(count) : 1;
}

static void collectPerCoreSnapshots(std::ifstream& statFile, unsigned numCores, std::vector<CpuTimes>& snapshots) {
    std::string cpu_str;
    CpuTimes snapshot_tmp;

    for (int i = 0; i < numCores; i++) {
        getline(statFile, cpu_str);
        snapshot_tmp = getCpuTimes(cpu_str);
        snapshots.push_back(snapshot_tmp);
    }
}

double calcUsage(double totalDelta, double idleDelta) {
    if(totalDelta == 0) {
        return 0.0;
    }

    return ((totalDelta - idleDelta) / totalDelta) * 100.0;
}

CpuUsage SystemInfo::getCpuUsage() const {
    // Sample the current CPU times
    std::string cpu_str;
    CpuTimes snapshot1, snapshot2;
    std::vector<CpuTimes> per_core_snapshot1;
    std::vector<CpuTimes> per_core_snapshot2;
    int num_cores = getLogicalCoreCount();
    CpuUsage stats;

    // Read first snapshot from /proc/stat
    std::ifstream statFile("/proc/stat");
    getline(statFile, cpu_str);
    snapshot1 = getCpuTimes(cpu_str);

    collectPerCoreSnapshots(statFile, num_cores, per_core_snapshot1);

    // Sleep for a short period to get the second snapshot
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    statFile.clear();
    statFile.seekg(0);

    // Read second snapshot from /proc/stat
    getline(statFile, cpu_str);
    snapshot2 = getCpuTimes(cpu_str);

    collectPerCoreSnapshots(statFile, num_cores, per_core_snapshot2);

    statFile.close();

    // Calculate CPU usage percentage
    double total_delta = snapshot2.total - snapshot1.total;
    double idle_delta = snapshot2.idle - snapshot1.idle;

    stats.totalUsage = calcUsage(total_delta, idle_delta);

    for (int i = 0; i < num_cores; i++) {
        double per_core_total_delta = per_core_snapshot2[i].total - per_core_snapshot1[i].total;
        double per_core_idle_delta = per_core_snapshot2[i].idle - per_core_snapshot1[i].idle;
        double per_cor_percent = calcUsage(per_core_total_delta, per_core_idle_delta);

        stats.perCoreUsage.push_back(per_cor_percent);
    }

    return stats;
}
