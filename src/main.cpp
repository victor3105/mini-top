#include <iostream>

#include "SystemInfo.h"

int main()
{
  SystemInfo info = SystemInfo();
  CpuUsage usage;

  while(1) {
    std::cout << "CPU load:\n";
    usage = info.getCpuUsage();
    std::cout << "Total usage = " << usage.totalUsage << "%\n";
    for (int i = 0; i < usage.perCoreUsage.size(); i++) {
      std::cout << "Core " << i << " usage = " << usage.perCoreUsage[i] << "%\n";
    }
  }

  return 0;
}
