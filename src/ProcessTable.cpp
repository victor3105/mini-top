#include "ProcessTable.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#include "SystemInfo.h"

namespace proctable {
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

namespace {
bool isNumber(const std::string_view s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}
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

ProcessInfo getProcessInfo(const std::string_view pid) {
  std::string commPath = "/proc/";
  commPath += pid;
  commPath += "/comm";
  std::string statusPath = "/proc/";
  statusPath += pid;
  statusPath += "/status";

  std::ifstream commFile(commPath);
  std::ifstream statusFile(statusPath);
  ProcessInfo result;

  std::string name;
  if (commFile) {
    std::getline(commFile, name);
  }

  std::string line;
  std::string state;
  uint64_t memory = 0;

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

static uint64_t procCpuTime(const std::string_view pid) {
  std::string path = "/proc/";
  path += pid;
  path += "/stat";
  std::ifstream file(path);
  uint64_t utime, stime;
  std::string token;

  // skip first 13 fields
  for (int i = 0; i < 13; ++i) file >> token;
  file >> utime >> stime;

  return utime + stime;
}

namespace {
void calcCpuUsage(ProcessInfo& procInfo,
                  std::unordered_map<std::string, uint64_t>& procTimes1,
                  const sysinfo::CpuTimes& totalSnapshot1,
                  const sysinfo::CpuTimes& totalSnapshot2, const int numCpus) {
  uint64_t procTime2 = procCpuTime(procInfo.pid);

  double deltaProc = procTime2 - procTimes1[procInfo.pid];
  double deltaTotal = totalSnapshot2.total - totalSnapshot1.total;

  procInfo.cpuUsed = (deltaProc / deltaTotal) * numCpus * 100.0;
}
}

std::vector<ProcessInfo> ProcessTable::getProcesses() const {
  std::vector<ProcessInfo> res;
  sysinfo::SystemInfo sysInfo = sysinfo::SystemInfo(this->snapshotsSleepMs);
  std::string cpuStr;
  std::ifstream statFile("/proc/stat");
  getline(statFile, cpuStr);
  sysinfo::CpuTimes totalSnapshot1 = sysInfo.getCpuTimes(cpuStr);
  std::unordered_map<std::string, uint64_t> procTimes1;

  for (const auto& entry : fs::directory_iterator("/proc")) {
    std::string filename = entry.path().filename().string();
    ProcessInfo info;

    if (entry.is_directory() && isNumber(filename)) {
      info = getProcessInfo(filename);
      res.push_back(std::move(info));
      procTimes1[filename] = procCpuTime(filename);
    }
  }

  std::this_thread::sleep_for(
      std::chrono::milliseconds(this->snapshotsSleepMs));

  statFile.clear();
  statFile.seekg(0);
  getline(statFile, cpuStr);
  sysinfo::CpuTimes totalSnapshot2 = sysInfo.getCpuTimes(cpuStr);

  int numCpus = std::thread::hardware_concurrency();
  std::for_each(res.begin(), res.end(), [&](ProcessInfo& x) {
    calcCpuUsage(x, procTimes1, totalSnapshot1, totalSnapshot2, numCpus);
  });

  std::sort(res.begin(), res.end(), [](ProcessInfo& a, ProcessInfo& b) {
    return a.cpuUsed > b.cpuUsed;
  });

  return res;
}
}  // namespace proctable
