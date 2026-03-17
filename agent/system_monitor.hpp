#pragma once

#include <string>
#include <vector>

struct ProcessInfo {
    int         pid;
    std::string name;
    double      cpuPercent;
    long        memKb;
};

struct SystemSnapshot {
    double             cpuUsagePercent;
    double             memUsedPercent;
    long               memTotalKb;
    long               memAvailableKb;
    double             uptimeSeconds;
    std::vector<ProcessInfo> topProcesses;
};

class SystemMonitor {
public:
    SystemMonitor();

    // Collect a fresh snapshot (CPU requires two samples spaced ~1 s apart).
    SystemSnapshot collect();

    // Serialise snapshot to a compact JSON string.
    static std::string toJson(const SystemSnapshot& snap);

private:
    struct CpuTimes {
        long long user, nice, system, idle, iowait, irq, softirq;
    };

    CpuTimes readCpuTimes();
    double   calcCpuUsage(const CpuTimes& a, const CpuTimes& b);
    void     readMemInfo(long& totalKb, long& availableKb);
    double   readUptime();
    std::vector<ProcessInfo> readTopProcesses(int n = 5);
};
