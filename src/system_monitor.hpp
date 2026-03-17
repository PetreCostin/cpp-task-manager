#pragma once

struct SystemStats {
    float cpu;     // CPU load (scaled from /proc/loadavg, approximation)
    float memory;  // Memory usage percentage
};

class SystemMonitor {
public:
    SystemStats getStats();
};
