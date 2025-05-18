#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

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

double SystemInfo::getCpuUsage() const {
    // Sample the current CPU times
    std::string cpu_str;
    CpuTimes snapshot1, snapshot2;

    // Read first snapshot from /proc/stat
    std::ifstream statFile("/proc/stat");
    getline(statFile, cpu_str);
    snapshot1 = getCpuTimes(cpu_str);

    // Sleep for a short period to get the second snapshot
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    statFile.clear();
    statFile.seekg(0);

    // Read second snapshot from /proc/stat
    getline(statFile, cpu_str);
    snapshot2 = getCpuTimes(cpu_str);

    statFile.close();

    // Calculate CPU usage percentage
    double total_delta = snapshot2.total - snapshot1.total;
    double idle_delta = snapshot2.idle - snapshot1.idle;

    return ((total_delta - idle_delta) / total_delta) * 100.0;
}
