#include <cxxopts.hpp>
#include <future>
#include <iostream>

#include "ProcessTable.h"
#include "SystemInfo.h"

int main(int argc, char *argv[]) {
  CpuUsage cpuUsage;
  MemoryUsage memUsage;
  std::vector<ProcessInfo> processes;
  // Percentage of overall refresh timeout to use in CPU/RAM threads
  constexpr unsigned thrSleepPercent = 10;

  cxxopts::Options options("mini-top", "Top-like system monitor");

  options.add_options()
      ("i,interval", "Refresh interval in milliseconds",
       cxxopts::value<unsigned>()->default_value("200"))
      ("n,nproc", "Number of processes to display",
       cxxopts::value<unsigned>()->default_value("10"))
      ("h,help", "Print help");

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  unsigned intervalMs = result["interval"].as<unsigned>();
  unsigned procNum = result["nproc"].as<unsigned>();

  ProcessTable procTable = ProcessTable(intervalMs / thrSleepPercent);
  SystemInfo sysInfo = SystemInfo(intervalMs / thrSleepPercent);

  while (1) {
    auto memFuture =
        std::async(std::launch::async, [info = sysInfo]() { return info.getMemoryUsage(); });

    auto cpuFuture =
        std::async(std::launch::async, [info = sysInfo]() { return info.getCpuUsage(); });

    auto procFuture = std::async(std::launch::async,
                                 [table = procTable]() { return table.getProcesses(); });

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
    procTable.printTableHeader();
    unsigned processesToShow =
        processes.size() < procNum ? processes.size() : procNum;
    for (int i = 0; i < processesToShow; i++) {
      std::cout << processes[i] << "\n";
    }

    // Or use std::this_thread::sleep_until instead
    std::this_thread::sleep_for(
        std::chrono::milliseconds(intervalMs - (intervalMs / thrSleepPercent)));
  }

  return 0;
}
