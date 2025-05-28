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

std::vector<ProcessInfo> ProcessTable::getProcesses() const {
  std::vector<ProcessInfo> res;

  for (const auto& entry : fs::directory_iterator("/proc")) {
    std::string filename = entry.path().filename().string();
    ProcessInfo info;

    if (entry.is_directory() && isNumber(filename)) {
      info = getProcessInfo(filename);
      res.push_back(info);
    }
  }

  return res;
}
