#pragma once

#include <fstream>
#include <string>

// ─── SystemStats ─────────────────────────────────────────────────────────────
struct SystemStats {
    double cpu;   // CPU usage as a percentage (0–100)
};

// ─── SystemMonitor ───────────────────────────────────────────────────────────
// Reads system resource statistics.  All methods are static so that any plugin
// can call them without holding an instance.
class SystemMonitor {
    struct CpuTimes {
        long user    = 0;
        long nice    = 0;
        long system  = 0;
        long idle    = 0;
        long iowait  = 0;
        long irq     = 0;
        long softirq = 0;
        long steal   = 0;
    };

    static CpuTimes readCpuTimes() {
        CpuTimes t;
        std::ifstream f("/proc/stat");
        if (!f.is_open())
            return t;
        std::string tag;
        f >> tag >> t.user >> t.nice >> t.system >> t.idle
          >> t.iowait >> t.irq >> t.softirq >> t.steal;
        return t;
    }

public:
    // Returns a snapshot of system stats.  Successive calls produce an
    // increasingly accurate CPU-usage figure because the measurement is based
    // on the delta between the current sample and the previous one.  The very
    // first call returns cpu == 0.0.
    static SystemStats getStats() {
        static CpuTimes prev = {};
        static bool     initialized = false;

        CpuTimes curr = readCpuTimes();

        double cpu = 0.0;
        if (initialized) {
            long total_prev = prev.user + prev.nice + prev.system + prev.idle
                            + prev.iowait + prev.irq + prev.softirq + prev.steal;
            long total_curr = curr.user + curr.nice + curr.system + curr.idle
                            + curr.iowait + curr.irq + curr.softirq + curr.steal;

            long idle_prev = prev.idle + prev.iowait;
            long idle_curr = curr.idle + curr.iowait;

            long totalDiff = total_curr - total_prev;
            long idleDiff  = idle_curr  - idle_prev;

            if (totalDiff > 0)
                cpu = (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;
        }

        prev        = curr;
        initialized = true;

        return {cpu};
    }
};
