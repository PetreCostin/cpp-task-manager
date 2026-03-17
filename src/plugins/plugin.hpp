#pragma once

#include <string>
#include <memory>

// ─── Plugin ───────────────────────────────────────────────────────────────────
// Abstract base class for all dashboard plugins.
// Plugins produce a one-line data string that can be displayed in the UI or
// served over the network.
class Plugin {
public:
    virtual ~Plugin() = default;

    // Human-readable plugin name.
    virtual std::string name()    const = 0;

    // Semantic version string (e.g. "1.0.0").
    virtual std::string version() const = 0;

    // Called once at startup – allocate resources, open files, etc.
    virtual void init() = 0;

    // Return the latest data string (must be cheap / non-blocking).
    virtual std::string getData() = 0;

    // Called once at shutdown – release resources.
    virtual void shutdown() = 0;
};

// ─── Plugin Factories ─────────────────────────────────────────────────────────
// Returns a heap-allocated CpuPlugin ready for use.
std::unique_ptr<Plugin> createCpuPlugin();
