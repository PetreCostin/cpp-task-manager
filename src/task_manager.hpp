#pragma once

#include <string>
#include <vector>
#include <mutex>

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
public:
    void addTask(const std::string& name, Priority p);
    void removeTask(size_t idx);
    void cycleStatus(size_t idx);
    void cyclePriority(size_t idx);
    void setStatus(size_t idx, TaskStatus s);
    int  count(TaskStatus s) const;

    // Returns a reference to the internal task list.
    // Must only be read from the thread that owns this TaskManager.
    const std::vector<Task>& getTasks() const { return tasks_; }

private:
    std::vector<Task> tasks_;
    int nextId_ = 1;
    mutable std::mutex mtx_;
};
