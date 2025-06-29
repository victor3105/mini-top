# mini-top

A lightweight, `top`-like system monitoring tool written in modern C++.  
Displays real-time CPU and memory usage, as well as a live list of running processes.

---

## Features

- Total CPU and RAM usage display
- Per-process stats: PID, name, CPU%, memory, state
- Per-core CPU usage support
- Refreshes periodically (like `top`)

---

## Build Instructions

### Prerequisites

- A Linux system (uses `/proc` for data)
- C++20 or higher
- `make` and `g++`

### Build

```bash
git clone https://github.com/victor3105/mini-top.git
cd mini-top
make
```
---

## Run

```
./build/monitor
```
You’ll see a real-time display of system and process statistics sorted by CPU usage.

---

## Possible Future Improvements
- Sort processes by memory usage
- Cross-platform support (macOS via sysctl)
- Better output formatting using `ncurses`
