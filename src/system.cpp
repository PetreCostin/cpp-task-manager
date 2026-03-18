#include "system.hpp"
#include <ctime>
#include <algorithm>

std::string currentTime() {
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y  %H:%M:%S", &ltm);
    return buf;
}

const char* priStr(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "HIGH    ";
        case Priority::MEDIUM: return "MEDIUM  ";
        case Priority::LOW:    return "LOW     ";
    }
    return "        ";
}

const char* statusStr(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return "PENDING   ";
        case TaskStatus::IN_PROGRESS: return "IN PROG   ";
        case TaskStatus::DONE:        return "DONE      ";
    }
    return "          ";
}

int nameWidth(int cols) {
    return std::max(10, cols - 47);
}
