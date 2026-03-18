#pragma once
#include <string>
#include <vector>

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

    void add(const std::string& name, Priority p);
    void remove(size_t idx);
    void cycleStatus(size_t idx);
    void cyclePriority(size_t idx);
    int count(TaskStatus s) const;
};
