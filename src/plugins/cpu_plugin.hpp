#pragma once

#include "plugin.hpp"
#include "../system/system_monitor.hpp"

class CPUPlugin : public Plugin {
public:
    std::string name() override {
        return "CPU Plugin";
    }

    std::string execute() override {
        auto stats = SystemMonitor::getStats();
        return "CPU: " + std::to_string(stats.cpu) + "%";
    }
};
