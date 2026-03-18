#include "system_monitor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ─── Internal CPU stat helper ─────────────────────────────────────────────────

struct CpuStat {
    long long user = 0, nice = 0, system = 0, idle = 0,
              iowait = 0, irq = 0, softirq = 0, steal = 0;

    long long total()  const { return user + nice + system + idle + iowait + irq + softirq + steal; }
    long long active() const { return user + nice + system + irq + softirq + steal; }
};

static CpuStat read_cpu_stat() {
    CpuStat s;
    std::ifstream f("/proc/stat");
    std::string label;
    f >> label >> s.user >> s.nice >> s.system >> s.idle
      >> s.iowait >> s.irq >> s.softirq >> s.steal;
    return s;
}

// ─── Internal memory reader ───────────────────────────────────────────────────

static void read_memory(double& used_mb, double& total_mb, double& percent) {
    std::ifstream f("/proc/meminfo");
    long long total_kb = 0, avail_kb = 0;
    std::string line;
    while (std::getline(f, line)) {
        long long val = 0;
        if (std::sscanf(line.c_str(), "MemTotal: %lld kB", &val) == 1)
            total_kb = val;
        else if (std::sscanf(line.c_str(), "MemAvailable: %lld kB", &val) == 1)
            avail_kb = val;
    }
    total_mb = total_kb / 1024.0;
    double used_kb = static_cast<double>(total_kb - avail_kb);
    used_mb  = used_kb / 1024.0;
    percent  = (total_kb > 0) ? (used_kb / static_cast<double>(total_kb)) * 100.0 : 0.0;
}

// ─── Internal process reader ──────────────────────────────────────────────────

static std::vector<ProcessInfo> read_processes(int top_n = 10) {
    std::vector<ProcessInfo> procs;

    DIR* dir = opendir("/proc");
    if (!dir) return procs;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Only numeric entries are PIDs
        bool is_pid = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') { is_pid = false; break; }
        }
        if (!is_pid) continue;

        int pid = std::atoi(entry->d_name);
        std::string status_path = "/proc/" + std::string(entry->d_name) + "/status";
        std::ifstream sf(status_path);
        if (!sf.is_open()) continue;

        ProcessInfo pi;
        pi.pid   = pid;
        pi.state = '?';
        pi.mem_kb = 0;

        std::string line;
        while (std::getline(sf, line)) {
            char buf[256];
            long val = 0;
            char state_ch = 0;
            if (std::sscanf(line.c_str(), "Name:\t%255s", buf) == 1)
                pi.name = buf;
            else if (std::sscanf(line.c_str(), "State:\t%c", &state_ch) == 1)
                pi.state = state_ch;
            else if (std::sscanf(line.c_str(), "VmRSS:\t%ld kB", &val) == 1)
                pi.mem_kb = val;
        }

        if (!pi.name.empty())
            procs.push_back(std::move(pi));
    }
    closedir(dir);

    // Sort descending by RSS, keep top_n
    std::sort(procs.begin(), procs.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.mem_kb > b.mem_kb;
              });
    if (static_cast<int>(procs.size()) > top_n)
        procs.resize(top_n);

    return procs;
}

// ─── Public API ───────────────────────────────────────────────────────────────

SystemMetrics collect_metrics() {
    static CpuStat prev{};

    CpuStat cur = read_cpu_stat();

    long long d_total  = cur.total()  - prev.total();
    long long d_active = cur.active() - prev.active();
    double cpu_usage = (d_total > 0)
                       ? (static_cast<double>(d_active) / d_total) * 100.0
                       : 0.0;
    prev = cur;

    SystemMetrics m;
    m.cpu_usage = cpu_usage;
    read_memory(m.mem_used_mb, m.mem_total_mb, m.mem_percent);
    m.top_processes = read_processes(10);
    return m;
}

// ─── JSON serialisation ───────────────────────────────────────────────────────

// Escape a string for embedding in JSON.
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20)  { /* skip control chars */ }
        else                out += static_cast<char>(c);
    }
    return out;
}

std::string metrics_to_json(const SystemMetrics& m) {
    std::ostringstream o;
    o.precision(2);
    o << std::fixed;

    o << "{"
      << "\"cpu_usage\":"    << m.cpu_usage    << ","
      << "\"mem_used_mb\":"  << m.mem_used_mb  << ","
      << "\"mem_total_mb\":" << m.mem_total_mb << ","
      << "\"mem_percent\":"  << m.mem_percent  << ","
      << "\"processes\":[";

    for (std::size_t i = 0; i < m.top_processes.size(); ++i) {
        const auto& p = m.top_processes[i];
        if (i > 0) o << ",";
        o << "{"
          << "\"pid\":"    << p.pid                     << ","
          << "\"name\":\"" << json_escape(p.name) << "\","
          << "\"state\":\"" << p.state             << "\","
          << "\"mem_kb\":" << p.mem_kb
          << "}";
    }

    o << "]}";
    return o.str();
}
