#include "task_manager.hpp"

#include <ctime>
#include <algorithm>

void TaskManager::addTask(const std::string& name, Priority p) {
    std::lock_guard<std::mutex> lock(mtx_);
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[11];
    strftime(buf, sizeof(buf), "%m/%d/%Y", &ltm);
    tasks_.push_back({nextId_++, name, p, TaskStatus::PENDING, buf});
}

void TaskManager::removeTask(size_t idx) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (idx < tasks_.size())
        tasks_.erase(tasks_.begin() + idx);
}

void TaskManager::cycleStatus(size_t idx) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (idx >= tasks_.size()) return;
    auto& s = tasks_[idx].status;
    s = static_cast<TaskStatus>((static_cast<int>(s) + 1) % 3);
}

void TaskManager::cyclePriority(size_t idx) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (idx >= tasks_.size()) return;
    auto& p = tasks_[idx].priority;
    p = static_cast<Priority>((static_cast<int>(p) + 1) % 3);
}

void TaskManager::setStatus(size_t idx, TaskStatus s) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (idx < tasks_.size())
        tasks_[idx].status = s;
}

int TaskManager::count(TaskStatus s) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return static_cast<int>(
        std::count_if(tasks_.begin(), tasks_.end(),
            [s](const Task& t){ return t.status == s; }));
}
