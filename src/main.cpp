#include <ncurses.h>
#include <clocale>
#include <cstdio>
#include <memory>
#include <string>
#include <algorithm>

#include "core/config.hpp"
#include "core/logger.hpp"
#include "task/task_manager.hpp"
#include "ui/ui.hpp"
#include "system/system_monitor.hpp"
#include "network/server.hpp"
#include "plugins/plugin.hpp"

// ─── Entry Point ──────────────────────────────────────────────────────────────
int main() {
    setlocale(LC_ALL, "");

    // Initialise logging
    Logger::instance().init("/tmp/cpp-task-manager.log");
    Logger::instance().info("Application started");

    // Start ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "This terminal does not support colors.\n");
        return 1;
    }

    initColors();

    // Pre-populate with sample tasks
    TaskManager tm;
    tm.add("Design the TUI dashboard layout",   Priority::HIGH);
    tm.add("Implement ncurses color scheme",     Priority::HIGH);
    tm.add("Add task creation dialog",           Priority::MEDIUM);
    tm.add("Write unit tests for TaskManager",   Priority::MEDIUM);
    tm.add("Update project documentation",       Priority::LOW);
    tm.tasks[1].status = TaskStatus::DONE;
    tm.tasks[2].status = TaskStatus::IN_PROGRESS;

    // Start background network server (best-effort; UI continues if it fails)
    Server server(9090);
    server.start(&tm);

    // Initialise CPU plugin
    std::unique_ptr<Plugin> cpuPlugin = createCpuPlugin();
    cpuPlugin->init();

    int sel        = 0;
    int scroll     = 0;
    std::string statusMsg;
    int statusTick = 0;

    halfdelay(HALFDELAY_TENTHS);   // 0.5 s refresh for real-time clock

    while (true) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        const int listH  = std::max(1, rows - 6);
        const int nTasks = static_cast<int>(tm.tasks.size());

        // Clamp selection
        if (nTasks > 0) sel = std::max(0, std::min(sel, nTasks - 1));
        else            sel = 0;

        // Scroll to keep selection visible
        if (sel < scroll)          scroll = sel;
        if (sel >= scroll + listH) scroll = sel - listH + 1;

        // Clear status message after timeout
        if (!statusMsg.empty() && ++statusTick >= STATUS_DURATION) {
            statusMsg.clear();
            statusTick = 0;
        }

        drawUI(tm, sel, scroll, statusMsg);

        int ch = getch();
        if (ch == ERR) continue;   // halfdelay timeout → just redraw

        switch (ch) {
            case 'q': case 'Q':
                server.stop();
                cpuPlugin->shutdown();
                Logger::instance().info("Application exited normally");
                endwin();
                return 0;

            case KEY_UP:   case 'k': case 'K':
                if (sel > 0) sel--;
                break;

            case KEY_DOWN: case 'j': case 'J':
                if (sel < nTasks - 1) sel++;
                break;

            case KEY_PPAGE:  // Page Up
                sel = std::max(0, sel - listH);
                break;

            case KEY_NPAGE:  // Page Down
                sel = std::min(nTasks - 1, sel + listH);
                break;

            case KEY_HOME:
                sel = 0;
                break;

            case KEY_END:
                if (nTasks > 0) sel = nTasks - 1;
                break;

            case 'a': case 'A': {
                cbreak();  // override halfdelay for blocking input
                std::string name;
                Priority prio = Priority::MEDIUM;
                if (dialogAdd(name, prio)) {
                    tm.add(name, prio);
                    sel = nTasks;  // point at new last task
                    statusMsg  = "[+] Task added successfully";
                    statusTick = 0;
                    Logger::instance().info("Task added: " + name);
                }
                halfdelay(HALFDELAY_TENTHS);
                break;
            }

            case 'd': case 'D':
                if (nTasks > 0) {
                    cbreak();
                    const std::string taskName =
                        tm.tasks[static_cast<size_t>(sel)].name;
                    if (dialogConfirmDelete(taskName)) {
                        tm.remove(static_cast<size_t>(sel));
                        Logger::instance().info("Task deleted: " + taskName);
                        const int newN = static_cast<int>(tm.tasks.size());
                        if (sel >= newN && sel > 0) sel--;
                        statusMsg  = "[-] Task deleted";
                        statusTick = 0;
                    }
                    halfdelay(HALFDELAY_TENTHS);
                }
                break;

            case '\n': case KEY_ENTER:
                if (nTasks > 0) {
                    tm.cycleStatus(static_cast<size_t>(sel));
                    statusMsg  = "[*] Status updated";
                    statusTick = 0;
                }
                break;

            case 'p': case 'P':
                if (nTasks > 0) {
                    tm.cyclePriority(static_cast<size_t>(sel));
                    statusMsg  = "[~] Priority changed";
                    statusTick = 0;
                }
                break;

            case KEY_RESIZE:
                clear();
                break;

            default:
                break;
        }
    }

    // Unreachable – quit is handled inside the loop.
    endwin();
    return 0;
}
