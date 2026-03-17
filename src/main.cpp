#include "ui.hpp"

// ─── Entry Point ──────────────────────────────────────────────────────────────
int main() {
    // Pre-populate with sample tasks
    TaskManager tm;
    tm.add("Design the TUI dashboard layout",   Priority::HIGH);
    tm.add("Implement ncurses color scheme",     Priority::HIGH);
    tm.add("Add task creation dialog",           Priority::MEDIUM);
    tm.add("Write unit tests for TaskManager",   Priority::MEDIUM);
    tm.add("Update project documentation",       Priority::LOW);
    tm.tasks[1].status = TaskStatus::DONE;
    tm.tasks[2].status = TaskStatus::IN_PROGRESS;

    UI::run(tm);
    return 0;
}
