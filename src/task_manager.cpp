#include "task_manager.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>

void TaskManager::add(const std::string& name, Priority p) {
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[11];
    strftime(buf, sizeof(buf), "%m/%d/%Y", &ltm);
    tasks.push_back({nextId++, name, p, TaskStatus::PENDING, buf});
}

void TaskManager::remove(size_t idx) {
    if (idx < tasks.size())
        tasks.erase(tasks.begin() + static_cast<std::ptrdiff_t>(idx));
}

void TaskManager::cycleStatus(size_t idx) {
    if (idx >= tasks.size()) return;
    auto& s = tasks[idx].status;
    s = static_cast<TaskStatus>((static_cast<int>(s) + 1) % 3);
}

void TaskManager::cyclePriority(size_t idx) {
    if (idx >= tasks.size()) return;
    auto& p = tasks[idx].priority;
    p = static_cast<Priority>((static_cast<int>(p) + 1) % 3);
}

int TaskManager::count(TaskStatus s) const {
    return static_cast<int>(
        std::count_if(tasks.begin(), tasks.end(),
            [s](const Task& t){ return t.status == s; }));
}
