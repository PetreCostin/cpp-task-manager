#include "logger.hpp"

#include <ctime>
#include <iostream>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(filename, std::ios::app);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &ltm);

    std::string entry = std::string(ts) + " [" + levelStr(level) + "] " + message + "\n";

    if (file_.is_open())
        file_ << entry << std::flush;
}

void Logger::debug(const std::string& msg)   { log(LogLevel::DEBUG,   msg); }
void Logger::info(const std::string& msg)    { log(LogLevel::INFO,    msg); }
void Logger::warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
void Logger::error(const std::string& msg)   { log(LogLevel::ERROR_LEVEL, msg); }

const char* Logger::levelStr(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR_LEVEL: return "ERROR";
    }
    return "     ";
}
