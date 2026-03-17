#include "plugin.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

// ─── CpuPlugin ────────────────────────────────────────────────────────────────
// Reports the overall CPU utilisation by diffing consecutive /proc/stat reads.
class CpuPlugin : public Plugin {
public:
    std::string name()    const override { return "CPU Plugin"; }
    std::string version() const override { return "1.0.0"; }

    void init() override {
        readCpuTimes(prevIdle_, prevTotal_);
    }

    std::string getData() override {
        long idle  = 0;
        long total = 0;
        readCpuTimes(idle, total);

        const long deltaIdle  = idle  - prevIdle_;
        const long deltaTotal = total - prevTotal_;

        prevIdle_  = idle;
        prevTotal_ = total;

        double usage = 0.0;
        if (deltaTotal > 0)
            usage = 100.0 * (1.0 - static_cast<double>(deltaIdle)
                                     / static_cast<double>(deltaTotal));

        std::ostringstream oss;
        oss << "CPU Usage: " << static_cast<int>(usage) << "%";
        return oss.str();
    }

    void shutdown() override {}

private:
    long prevIdle_  = 0;
    long prevTotal_ = 0;

    // Read the first "cpu" line of /proc/stat and compute aggregate
    // idle and total jiffies.
    static void readCpuTimes(long& idle, long& total) {
        std::ifstream f("/proc/stat");
        if (!f.is_open()) { idle = total = 0; return; }

        std::string label;
        long user = 0, nice = 0, system = 0, idleJ = 0,
             iowait = 0, irq = 0, softirq = 0;
        f >> label >> user >> nice >> system >> idleJ
          >> iowait >> irq >> softirq;

        idle  = idleJ + iowait;
        total = user + nice + system + idleJ + iowait + irq + softirq;
    }
};

// ─── Factory ──────────────────────────────────────────────────────────────────
std::unique_ptr<Plugin> createCpuPlugin() {
    return std::make_unique<CpuPlugin>();
}

