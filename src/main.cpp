#include <iostream>

#include "SystemInfo.h"

int main()
{
  SystemInfo info = SystemInfo();

  while(1) {
    std::cout << "CPU load: " << info.getCpuUsage() << "%\n";
  }

  return 0;
}
