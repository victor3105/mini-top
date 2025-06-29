#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include <iomanip>
#include <string>
#include <vector>

// R	Running	Actively running on a CPU or ready to run.
// S	Sleeping (interruptible)	Waiting for an event (e.g., input), but
// can be woken by signals. D	Sleeping (uninterruptible)	Waiting on I/O;
// can't be woken by signals. Often device or disk wait. T	Stopped	Stopped
// by signal (e.g., SIGSTOP or debugger like gdb). Z	Zombie	Process
// terminated, but parent hasn't read its exit status (via wait()). X	Dead
// Shouldn't normally appear — indicates a dead/unreachable task (rare). I
// Idle (kernel threads only)	Idle kernel thread (since Linux 5.14+).
enum class ProcessState {
  Running,    // 'R'
  Sleeping,   // 'S'
  DiskSleep,  // 'D'
  Stopped,    // 'T'
  Zombie,     // 'Z'
  Dead,       // 'X'
  Idle,       // 'I'
  Unknown,    // For any unexpected state
};

std::ostream& operator<<(std::ostream& os, const ProcessState& state);

struct ProcessInfo {
  std::string pid;
  std::string name;
  ProcessState state;
  double cpuUsed;
  unsigned long memUsedKB;
};

std::ostream& operator<<(std::ostream& os, const ProcessInfo& info);

class ProcessTable {
 public:
  std::vector<ProcessInfo> getProcesses() const;
  void printTableHeader() const;
  ProcessTable(unsigned sleepMs = 100) : snapshotsSleepMs(sleepMs) {}

 private:
  unsigned snapshotsSleepMs;
};

#endif /* PROCESS_TABLE_H */
