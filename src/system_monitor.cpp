#include "system_monitor.hpp"

#include <cstring>
#include <ctime>

std::string currentTime() {
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y  %H:%M:%S", &ltm);
    return buf;
}
