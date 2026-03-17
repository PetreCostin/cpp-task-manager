#pragma once

#include <string>

// ─── Aggregated System Statistics ─────────────────────────────────────────────
struct SystemStats {
    double cpuUsage;      // percentage 0–100
    long   memTotal;      // kB
    long   memAvailable;  // kB
    int    processCount;  // number of running processes
};

// ─── SystemMonitor ────────────────────────────────────────────────────────────
class SystemMonitor {
public:
    // Collect a fresh snapshot (reads /proc/stat and /proc/meminfo).
    SystemStats getStats();

    // Return the CPU model string from /proc/cpuinfo.
    std::string getCpuModel();

private:
    long prevIdleTime_  = 0;
    long prevTotalTime_ = 0;
};
