#include "system_monitor.hpp"
#include <fstream>
#include <string>
#include <cstdio>

SystemStats SystemMonitor::getStats() {
    SystemStats stats{0, 0};

    // CPU (simple approximation)
    std::ifstream cpuFile("/proc/loadavg");
    if (cpuFile.is_open()) {
        cpuFile >> stats.cpu;
        stats.cpu *= 100; // scale
    }

    // Memory
    std::ifstream memFile("/proc/meminfo");
    std::string line;
    float total = 0, free = 0;

    if (memFile.is_open()) {
        bool foundTotal = false, foundAvail = false;
        while (getline(memFile, line) && !(foundTotal && foundAvail)) {
            if (!foundTotal && line.find("MemTotal") != std::string::npos) {
                sscanf(line.c_str(), "MemTotal: %f", &total);
                foundTotal = true;
            } else if (!foundAvail && line.find("MemAvailable") != std::string::npos) {
                sscanf(line.c_str(), "MemAvailable: %f", &free);
                foundAvail = true;
            }
        }
    }

    if (total > 0)
        stats.memory = ((total - free) / total) * 100;

    return stats;
}
