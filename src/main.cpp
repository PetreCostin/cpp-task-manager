#include "task_manager.hpp"
#include "ui.hpp"

#include <ncurses.h>
#include <clocale>
#include <cstdio>

// ─── Entry Point ──────────────────────────────────────────────────────────────
int main() {
    setlocale(LC_ALL, "");
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

    start_color();
    use_default_colors();

    init_pair(CP_HEADER,   COLOR_BLACK,   COLOR_CYAN);
    init_pair(CP_SELECTED, COLOR_BLACK,   COLOR_WHITE);
    init_pair(CP_HIGH,     COLOR_RED,     -1);
    init_pair(CP_MEDIUM,   COLOR_YELLOW,  -1);
    init_pair(CP_LOW,      COLOR_GREEN,   -1);
    init_pair(CP_DONE,     COLOR_GREEN,   -1);
    init_pair(CP_NORMAL,   COLOR_WHITE,   -1);
    init_pair(CP_INPROG,   COLOR_YELLOW,  -1);
    init_pair(CP_STATS,    COLOR_CYAN,    -1);
    init_pair(CP_KEY,      COLOR_WHITE,   COLOR_CYAN);
    init_pair(CP_TITLE,    COLOR_CYAN,    -1);
    init_pair(CP_DIALOG,   COLOR_WHITE,   -1);

    // Pre-populate with sample tasks
    TaskManager tm;
    tm.add("Design the TUI dashboard layout",   Priority::HIGH);
    tm.add("Implement ncurses color scheme",     Priority::HIGH);
    tm.add("Add task creation dialog",           Priority::MEDIUM);
    tm.add("Write unit tests for TaskManager",   Priority::MEDIUM);
    tm.add("Update project documentation",       Priority::LOW);
    tm.tasks[1].status = TaskStatus::DONE;
    tm.tasks[2].status = TaskStatus::IN_PROGRESS;

    int sel   = 0;
    int scroll = 0;
    std::string statusMsg;
    int statusTick = 0;
    const int STATUS_DURATION = 20;

    halfdelay(5);   // 0.5 s refresh for real-time clock

    while (true) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        const int listH  = std::max(1, rows - 6);
        const int nTasks = static_cast<int>(tm.tasks.size());

        // Clamp selection
        if (nTasks > 0) sel = std::max(0, std::min(sel, nTasks - 1));
        else            sel = 0;

        // Scroll to keep selection visible
        if (sel < scroll)              scroll = sel;
        if (sel >= scroll + listH)     scroll = sel - listH + 1;

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
                cbreak();   // override halfdelay for blocking input
                std::string name;
                Priority prio = Priority::MEDIUM;
                if (dialogAdd(name, prio)) {
                    tm.add(name, prio);
                    sel = nTasks;   // point at new last task
                    statusMsg  = "[+] Task added successfully";
                    statusTick = 0;
                }
                halfdelay(5);
                break;
            }

            case 'd': case 'D':
                if (nTasks > 0) {
                    cbreak();
                    if (dialogConfirmDelete(tm.tasks[sel].name)) {
                        tm.remove(sel);
                        const int newN = static_cast<int>(tm.tasks.size());
                        if (sel >= newN && sel > 0) sel--;
                        statusMsg  = "[-] Task deleted";
                        statusTick = 0;
                    }
                    halfdelay(5);
                }
                break;

            case '\n': case KEY_ENTER:
                if (nTasks > 0) {
                    tm.cycleStatus(sel);
                    statusMsg  = "[*] Status updated";
                    statusTick = 0;
                }
                break;

            case 'p': case 'P':
                if (nTasks > 0) {
                    tm.cyclePriority(sel);
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

    endwin();
    return 0;
}
