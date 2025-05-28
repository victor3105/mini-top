#include <iostream>

#include "ProcessTable.h"
#include "SystemInfo.h"

int main() {
  SystemInfo info = SystemInfo();
  CpuUsage cpuUsage;
  MemoryUsage memUsage;
  std::vector<ProcessInfo> processes;
  ProcessTable procTable;

  while (1) {
    std::cout << "CPU load:\n";
    cpuUsage = info.getCpuUsage();
    std::cout << "Total usage = " << cpuUsage.totalUsage << "%\n";
    for (int i = 0; i < cpuUsage.perCoreUsage.size(); i++) {
      std::cout << "Core " << i << " usage = " << cpuUsage.perCoreUsage[i]
                << "%\n";
    }
    memUsage = info.getMemoryUsage();

    std::cout << "Total RAM: " << memUsage.totalKB << " kB\n";
    std::cout << "Used RAM: " << memUsage.usedKB << " kB\n";
    std::cout << "Available RAM: " << memUsage.availableKB << " kB\n";
    std::cout << "RAM usage: " << memUsage.usedPercent << "%\n";

    processes = procTable.getProcesses();
    std::cout << "Active processes: " << processes.size() << "\n\n";
    for (auto x : processes) {
      std::cout << x << "\n";
    }

    break;
  }

  return 0;
}
