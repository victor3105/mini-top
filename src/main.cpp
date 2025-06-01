#include <future>
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
    auto memFuture =
        std::async(std::launch::async, [&]() { return info.getMemoryUsage(); });

    auto cpuFuture =
        std::async(std::launch::async, [&]() { return info.getCpuUsage(); });

    auto procFuture = std::async(std::launch::async,
                                 [&]() { return procTable.getProcesses(); });

    cpuUsage = cpuFuture.get();
    memUsage = memFuture.get();
    processes = procFuture.get();

    std::system("clear");

    std::cout << "CPU load:\n";

    std::cout << "Total usage = " << cpuUsage.totalUsage << "%\n";
    for (int i = 0; i < cpuUsage.perCoreUsage.size(); i++) {
      std::cout << "Core " << i << " usage = " << cpuUsage.perCoreUsage[i]
                << "%\n";
    }

    std::cout << "Total RAM: " << memUsage.totalKB << " kB\n";
    std::cout << "Used RAM: " << memUsage.usedKB << " kB\n";
    std::cout << "Available RAM: " << memUsage.availableKB << " kB\n";
    std::cout << "RAM usage: " << memUsage.usedPercent << "%\n";

    std::cout << "Active processes: " << processes.size() << "\n\n";
    for (int i = 0; i < processes.size(); i++) {
      std::cout << processes[i] << "\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return 0;
}
