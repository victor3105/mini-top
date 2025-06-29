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
     << std::setprecision(2) << info.cpuUsed << std::setw(10) << info.memUsedKB;
  return os;
}

void ProcessTable::printTableHeader() const {
  std::cout << std::left << std::setw(8) << "PID" << std::setw(40) << "Name"
            << std::setw(10) << "State" << std::setw(6) << "% CPU"
            << std::setw(10) << "RAM KB" << "\n";
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
  std::string commPath = "/proc/" + pid + "/comm";
  std::string statusPath = "/proc/" + pid + "/status";

  std::ifstream commFile(commPath);
  std::ifstream statusFile(statusPath);
  ProcessInfo result;

  std::string name;
  if (commFile) {
    std::getline(commFile, name);
  }

  std::string line;
  std::string state;
  unsigned long memory = 0;

  while (std::getline(statusFile, line)) {
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
  SystemInfo sysInfo = SystemInfo(this->snapshotsSleepMs);
  std::string cpuStr;
  std::ifstream statFile("/proc/stat");
  getline(statFile, cpuStr);
  CpuTimes totalSnapshot1 = sysInfo.getCpuTimes(cpuStr);
  std::unordered_map<std::string, unsigned long> procTimes1;
  int numCpus = std::thread::hardware_concurrency();

  for (const auto& entry : fs::directory_iterator("/proc")) {
    std::string filename = entry.path().filename().string();
    ProcessInfo info;

    if (entry.is_directory() && isNumber(filename)) {
      info = getProcessInfo(filename);
      res.push_back(info);
      procTimes1[filename] = procCpuTime(filename);
    }
  }

  std::this_thread::sleep_for(
      std::chrono::milliseconds(this->snapshotsSleepMs));

  statFile.clear();
  statFile.seekg(0);
  getline(statFile, cpuStr);
  CpuTimes totalSnapshot2 = sysInfo.getCpuTimes(cpuStr);

  for (auto& x : res) {
    unsigned long procTime2 = procCpuTime(x.pid);

    double deltaProc = procTime2 - procTimes1[x.pid];
    double deltaTotal = totalSnapshot2.total - totalSnapshot1.total;

    x.cpuUsed = (deltaProc / deltaTotal) * numCpus * 100.0;
  }

  std::sort(res.begin(), res.end(), [](ProcessInfo& a, ProcessInfo& b) {
    return a.cpuUsed > b.cpuUsed;
  });

  return res;
}
