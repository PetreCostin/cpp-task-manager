#include "logger.hpp"
#include <iostream>
#include <fstream>

static std::ofstream logFile("app.log", std::ios::app);

void Logger::info(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
    if (logFile.is_open())
        logFile << "[INFO] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
    if (logFile.is_open())
        logFile << "[ERROR] " << msg << std::endl;
}
