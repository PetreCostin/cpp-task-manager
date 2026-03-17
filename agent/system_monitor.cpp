#include "system_monitor.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

// ─── Constructor ──────────────────────────────────────────────────────────────
SystemMonitor::SystemMonitor() {}

// ─── collect ─────────────────────────────────────────────────────────────────
SystemSnapshot SystemMonitor::collect() {
    CpuTimes t1 = readCpuTimes();
    ::usleep(500'000);  // 0.5 s sample window
    CpuTimes t2 = readCpuTimes();

    SystemSnapshot snap;
    snap.cpuUsagePercent = calcCpuUsage(t1, t2);
    readMemInfo(snap.memTotalKb, snap.memAvailableKb);
    snap.memUsedPercent = snap.memTotalKb > 0
        ? 100.0 * (snap.memTotalKb - snap.memAvailableKb) / snap.memTotalKb
        : 0.0;
    snap.uptimeSeconds  = readUptime();
    snap.topProcesses   = readTopProcesses(5);
    return snap;
}

// ─── toJson ───────────────────────────────────────────────────────────────────
std::string SystemMonitor::toJson(const SystemSnapshot& s) {
    std::ostringstream o;
    o << "{"
      << "\"cpuUsagePercent\":"  << s.cpuUsagePercent  << ","
      << "\"memUsedPercent\":"   << s.memUsedPercent   << ","
      << "\"memTotalKb\":"       << s.memTotalKb       << ","
      << "\"memAvailableKb\":"   << s.memAvailableKb   << ","
      << "\"uptimeSeconds\":"    << s.uptimeSeconds    << ","
      << "\"topProcesses\":[";

    for (size_t i = 0; i < s.topProcesses.size(); ++i) {
        const auto& p = s.topProcesses[i];
        if (i) o << ",";
        // Escape the process name to keep JSON valid
        std::string safeName;
        for (char c : p.name) {
            if (c == '"' || c == '\\') safeName += '\\';
            safeName += c;
        }
        o << "{"
          << "\"pid\":"        << p.pid        << ","
          << "\"name\":\""     << safeName     << "\","
          << "\"cpuPercent\":" << p.cpuPercent << ","
          << "\"memKb\":"      << p.memKb
          << "}";
    }
    o << "]}";
    return o.str();
}

// ─── Private helpers ──────────────────────────────────────────────────────────
SystemMonitor::CpuTimes SystemMonitor::readCpuTimes() {
    std::ifstream f("/proc/stat");
    if (!f) throw std::runtime_error("Cannot open /proc/stat");

    std::string label;
    CpuTimes t{};
    f >> label >> t.user >> t.nice >> t.system >> t.idle
      >> t.iowait >> t.irq >> t.softirq;
    return t;
}

double SystemMonitor::calcCpuUsage(const CpuTimes& a, const CpuTimes& b) {
    long long idleDelta  = (b.idle + b.iowait) - (a.idle + a.iowait);
    long long totalDelta = (b.user + b.nice + b.system + b.idle + b.iowait
                          + b.irq + b.softirq)
                         - (a.user + a.nice + a.system + a.idle + a.iowait
                          + a.irq + a.softirq);
    if (totalDelta <= 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(idleDelta) / totalDelta);
}

void SystemMonitor::readMemInfo(long& totalKb, long& availableKb) {
    std::ifstream f("/proc/meminfo");
    if (!f) throw std::runtime_error("Cannot open /proc/meminfo");

    totalKb = availableKb = 0;
    std::string key;
    long value;
    std::string unit;
    while (f >> key >> value) {
        f >> unit;  // consume optional "kB"
        if (key == "MemTotal:")     totalKb     = value;
        if (key == "MemAvailable:") availableKb = value;
        if (totalKb && availableKb) break;
    }
}

double SystemMonitor::readUptime() {
    std::ifstream f("/proc/uptime");
    if (!f) return 0.0;
    double uptime;
    f >> uptime;
    return uptime;
}

std::vector<ProcessInfo> SystemMonitor::readTopProcesses(int n) {
    std::vector<ProcessInfo> procs;

    DIR* dir = opendir("/proc");
    if (!dir) return procs;

    long clkTck = sysconf(_SC_CLK_TCK);
    if (clkTck <= 0) clkTck = 100;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Only numeric directories are process entries
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) continue;
        bool allDigit = true;
        for (const char* p = entry->d_name; *p; ++p)
            if (*p < '0' || *p > '9') { allDigit = false; break; }
        if (!allDigit || entry->d_name[0] == '\0') continue;

        int pid = std::stoi(entry->d_name);

        // Read process name from /proc/<pid>/comm
        std::string commPath = "/proc/" + std::string(entry->d_name) + "/comm";
        std::ifstream commF(commPath);
        if (!commF) continue;
        std::string name;
        std::getline(commF, name);

        // Read stat for CPU times and memory
        std::string statPath = "/proc/" + std::string(entry->d_name) + "/stat";
        std::ifstream statF(statPath);
        if (!statF) continue;

        // Fields in /proc/<pid>/stat (space-separated)
        // 1:pid 2:comm 3:state 4:ppid ... 14:utime 15:stime ... 24:rss
        long utime = 0, stime = 0, rss = 0;
        std::string line;
        std::getline(statF, line);
        // Skip past closing ')' which ends the comm field
        size_t rpar = line.rfind(')');
        if (rpar == std::string::npos) continue;
        std::istringstream ss(line.substr(rpar + 2));
        std::string token;
        for (int field = 3; field <= 24; ++field) {
            if (!(ss >> token)) break;
            if (field == 14) utime = std::stol(token);
            if (field == 15) stime = std::stol(token);
            if (field == 24) rss   = std::stol(token);
        }

        ProcessInfo pi;
        pi.pid        = pid;
        pi.name       = name;
        pi.cpuPercent = static_cast<double>(utime + stime) / clkTck;
        pi.memKb      = rss * (sysconf(_SC_PAGESIZE) / 1024);
        procs.push_back(pi);
    }
    closedir(dir);

    // Sort by memory descending (best proxy without inter-sample delta)
    std::sort(procs.begin(), procs.end(),
        [](const ProcessInfo& a, const ProcessInfo& b){
            return a.memKb > b.memKb;
        });

    if (static_cast<int>(procs.size()) > n)
        procs.resize(n);
    return procs;
}
