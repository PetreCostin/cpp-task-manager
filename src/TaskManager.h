#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>

// ─── Domain Types ────────────────────────────────────────────────────────────
enum class Priority   { LOW = 0, MEDIUM = 1, HIGH = 2 };
enum class TaskStatus { PENDING = 0, IN_PROGRESS = 1, DONE = 2 };

struct Task {
    int         id;
    std::string name;
    Priority    priority;
    TaskStatus  status;
    std::string created;   // "MM/DD/YYYY"
};

// ─── TaskManager ─────────────────────────────────────────────────────────────
class TaskManager {
    int nextId = 1;
public:
    std::vector<Task> tasks;

    void add(const std::string& name, Priority p) {
        time_t now = time(nullptr);
        struct tm ltm {};
        localtime_r(&now, &ltm);
        char buf[11];
        strftime(buf, sizeof(buf), "%m/%d/%Y", &ltm);
        tasks.push_back({nextId++, name, p, TaskStatus::PENDING, buf});
    }

    void remove(size_t idx) {
        if (idx < tasks.size())
            tasks.erase(tasks.begin() + idx);
    }

    void cycleStatus(size_t idx) {
        if (idx >= tasks.size()) return;
        auto& s = tasks[idx].status;
        s = static_cast<TaskStatus>((static_cast<int>(s) + 1) % 3);
    }

    void cyclePriority(size_t idx) {
        if (idx >= tasks.size()) return;
        auto& p = tasks[idx].priority;
        p = static_cast<Priority>((static_cast<int>(p) + 1) % 3);
    }

    int count(TaskStatus s) const {
        return static_cast<int>(
            std::count_if(tasks.begin(), tasks.end(),
                [s](const Task& t){ return t.status == s; }));
    }
};
