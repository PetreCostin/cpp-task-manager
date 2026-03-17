#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

// ─── CPU Statistics ───────────────────────────────────────────────────────────
struct CpuStats {
    long long user = 0, nice = 0, sys = 0, idle = 0,
              iowait = 0, irq = 0, softirq = 0, steal = 0;

    long long total() const {
        return user + nice + sys + idle + iowait + irq + softirq + steal;
    }
    long long busy() const { return total() - idle - iowait; }
};

// ─── Memory Statistics ────────────────────────────────────────────────────────
struct MemInfo {
    long long total     = 0;   // kB
    long long available = 0;   // kB

    long long used()        const { return total - available; }
    int       usedPercent() const {
        return (total > 0) ? static_cast<int>(used() * 100 / total) : 0;
    }
};

// ─── System Monitor ───────────────────────────────────────────────────────────
class SystemMonitor {
    CpuStats prev{};
    bool     hasPrev = false;

    static bool readCpuStats(CpuStats& s) {
        std::ifstream f("/proc/stat");
        if (!f.is_open()) return false;
        std::string line;
        while (std::getline(f, line)) {
            if (line.find("cpu ") == 0) {
                std::istringstream ss(line.substr(4));
                ss >> s.user >> s.nice >> s.sys >> s.idle
                   >> s.iowait >> s.irq >> s.softirq >> s.steal;
                return true;
            }
        }
        return false;
    }

    static bool readMemInfo(MemInfo& m) {
        std::ifstream f("/proc/meminfo");
        if (!f.is_open()) return false;
        m = {};
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            std::string key;
            long long   val = 0;
            ss >> key >> val;
            if      (key == "MemTotal:")     m.total     = val;
            else if (key == "MemAvailable:") m.available = val;
            if (m.total > 0 && m.available > 0) break;
        }
        return m.total > 0;
    }

public:
    float   cpuUsage = 0.0f;
    MemInfo mem{};

    void update() {
        CpuStats cur;
        if (readCpuStats(cur)) {
            if (hasPrev) {
                long long dtotal = cur.total() - prev.total();
                long long dbusy  = cur.busy()  - prev.busy();
                cpuUsage = (dtotal > 0)
                    ? 100.0f * static_cast<float>(dbusy) / static_cast<float>(dtotal)
                    : 0.0f;
                cpuUsage = std::max(0.0f, std::min(100.0f, cpuUsage));
            }
            prev    = cur;
            hasPrev = true;
        }
        readMemInfo(mem);
    }
};
