#pragma once

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR_LEVEL };

class Logger {
public:
    static Logger& instance();

    // Open (or reopen) the log file; call once at startup.
    void init(const std::string& filename);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream file_;
    std::mutex    mutex_;

    static const char* levelStr(LogLevel level);
};
