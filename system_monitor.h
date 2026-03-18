#pragma once

#include <string>
#include <vector>

// ─── Data Structures ─────────────────────────────────────────────────────────

struct ProcessInfo {
    int         pid;
    std::string name;
    char        state;      // R, S, D, Z, T, …
    long        mem_kb;     // VmRSS from /proc/[pid]/status
};

struct SystemMetrics {
    double cpu_usage;       // 0.0 – 100.0 %
    double mem_used_mb;
    double mem_total_mb;
    double mem_percent;     // 0.0 – 100.0 %
    std::vector<ProcessInfo> top_processes;  // top 10 by RSS
};

// ─── API ─────────────────────────────────────────────────────────────────────

// Collect a snapshot of system metrics.
// Internally keeps a static prev-CPU-stat so the first call returns 0 % CPU.
SystemMetrics collect_metrics();

// Serialise metrics to a compact JSON string.
std::string metrics_to_json(const SystemMetrics& m);
