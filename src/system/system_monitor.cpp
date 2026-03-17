#include "system_monitor.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

// ─── CPU usage via /proc/stat ─────────────────────────────────────────────────
SystemStats SystemMonitor::getStats() {
    SystemStats stats{};

    // ── CPU usage ─────────────────────────────────────────────────────────────
    {
        std::ifstream f("/proc/stat");
        if (f.is_open()) {
            std::string label;
            long user = 0, nice = 0, system = 0, idle = 0,
                 iowait = 0, irq = 0, softirq = 0;
            f >> label >> user >> nice >> system >> idle
              >> iowait >> irq >> softirq;

            long idleTime  = idle + iowait;
            long totalTime = user + nice + system + idle + iowait + irq + softirq;

            long deltaIdle  = idleTime  - prevIdleTime_;
            long deltaTotal = totalTime - prevTotalTime_;

            if (deltaTotal > 0)
                stats.cpuUsage = 100.0 * (1.0 - static_cast<double>(deltaIdle)
                                                  / static_cast<double>(deltaTotal));
            else
                stats.cpuUsage = 0.0;

            prevIdleTime_  = idleTime;
            prevTotalTime_ = totalTime;
        }
    }

    // ── Memory info via /proc/meminfo ─────────────────────────────────────────
    {
        std::ifstream f("/proc/meminfo");
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                if (line.rfind("MemTotal:", 0) == 0) {
                    std::istringstream ss(line.substr(9));
                    ss >> stats.memTotal;
                } else if (line.rfind("MemAvailable:", 0) == 0) {
                    std::istringstream ss(line.substr(13));
                    ss >> stats.memAvailable;
                }
            }
        }
    }

    // ── Process count via /proc/stat ─────────────────────────────────────────
    {
        std::ifstream f("/proc/stat");
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                if (line.rfind("procs_running", 0) == 0) {
                    std::istringstream ss(line.substr(14));
                    ss >> stats.processCount;
                    break;
                }
            }
        }
    }

    return stats;
}

// ─── CPU model string ─────────────────────────────────────────────────────────
std::string SystemMonitor::getCpuModel() {
    std::ifstream f("/proc/cpuinfo");
    if (!f.is_open()) return "Unknown";

    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("model name", 0) == 0) {
            const auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string model = line.substr(pos + 1);
                // Trim leading spaces
                const auto start = model.find_first_not_of(' ');
                if (start != std::string::npos)
                    return model.substr(start);
            }
        }
    }
    return "Unknown";
}
