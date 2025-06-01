#include "ProcessTable.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

#include "SystemInfo.h"

std::ostream& operator<<(std::ostream& os, const ProcessState& state) {
  switch (state) {
    case ProcessState::Running:
      return os << "Running";
    case ProcessState::Sleeping:
      return os << "Sleeping";
    case ProcessState::DiskSleep:
      return os << "Disk Sleep";
    case ProcessState::Stopped:
      return os << "Stopped";
    case ProcessState::Zombie:
      return os << "Zombie";
    case ProcessState::Dead:
      return os << "Dead";
    case ProcessState::Idle:
      return os << "Idle";
    default:
      return os << "Unknown";
  }
}

std::ostream& operator<<(std::ostream& os, const ProcessInfo& info) {
  os << std::left << std::setw(8) << info.pid << std::setw(40) << info.name
     << std::setw(10) << info.state << std::setw(6) << std::fixed
     << std::setprecision(2) << info.cpuUsed << "%\t" << std::setw(10)
     << info.memUsedKB << " KB";
  return os;
}

namespace fs = std::filesystem;

static bool isNumber(const std::string& s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

static ProcessState stateToProcessState(char state) {
  switch (state) {
    case 'R':
      return ProcessState::Running;
    case 'S':
      return ProcessState::Sleeping;
    case 'D':
      return ProcessState::DiskSleep;
    case 'T':
      return ProcessState::Stopped;
    case 'Z':
      return ProcessState::Zombie;
    case 'X':
      return ProcessState::Dead;
    case 'I':
      return ProcessState::Idle;
    default:
      return ProcessState::Unknown;
  }
}

ProcessInfo getProcessInfo(const std::string& pid) {
  std::string comm_path = "/proc/" + pid + "/comm";
  std::string status_path = "/proc/" + pid + "/status";

  std::ifstream comm_file(comm_path);
  std::ifstream status_file(status_path);
  ProcessInfo result;

  std::string name;
  if (comm_file) {
    std::getline(comm_file, name);
  }

  std::string line;
  std::string state;
  unsigned long memory = 0;

  while (std::getline(status_file, line)) {
    std::istringstream iss(line);
    std::string label;

    if (line.rfind("State:", 0) == 0) iss >> label >> state;
    if (line.rfind("VmRSS:", 0) == 0) iss >> label >> memory;
  }

  result.name = name;
  result.pid = pid;
  result.state = stateToProcessState(state.data()[0]);
  result.memUsedKB = memory;

  return result;
}

static unsigned long procCpuTime(const std::string& pid) {
  std::ifstream file("/proc/" + pid + "/stat");
  unsigned long utime, stime;
  std::string token;

  // skip first 13 fields
  for (int i = 0; i < 13; ++i) file >> token;
  file >> utime >> stime;

  return utime + stime;
}

std::vector<ProcessInfo> ProcessTable::getProcesses() const {
  std::vector<ProcessInfo> res;
  SystemInfo sysInfo = SystemInfo();
  std::string cpu_str;
  std::ifstream statFile("/proc/stat");
  getline(statFile, cpu_str);
  CpuTimes total_snapshot1 = sysInfo.getCpuTimes(cpu_str);
  std::unordered_map<std::string, unsigned long> proc_times1;
  int num_cpus = std::thread::hardware_concurrency();

  for (const auto& entry : fs::directory_iterator("/proc")) {
    std::string filename = entry.path().filename().string();
    ProcessInfo info;

    if (entry.is_directory() && isNumber(filename)) {
      info = getProcessInfo(filename);
      res.push_back(info);
      proc_times1[filename] = procCpuTime(filename);
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  statFile.clear();
  statFile.seekg(0);
  getline(statFile, cpu_str);
  CpuTimes total_snapshot2 = sysInfo.getCpuTimes(cpu_str);

  for (auto& x : res) {
    unsigned long procTime2 = procCpuTime(x.pid);

    double delta_proc = procTime2 - proc_times1[x.pid];
    double delta_total = total_snapshot2.total - total_snapshot1.total;

    x.cpuUsed = (delta_proc / delta_total) * num_cpus * 100.0;
  }

  std::sort(res.begin(), res.end(), [](ProcessInfo& a, ProcessInfo& b) {
    return a.cpuUsed > b.cpuUsed;
  });

  return res;
}
