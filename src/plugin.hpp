#pragma once

#include <string>

// ─── Plugin ───────────────────────────────────────────────────────────────────
// Abstract base class for all plugins.
class Plugin {
public:
    virtual ~Plugin() = default;

    // Human-readable plugin name.
    virtual std::string name() = 0;

    // Run the plugin and return its output as a string.
    virtual std::string execute() = 0;
};
