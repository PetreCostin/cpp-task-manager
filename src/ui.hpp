#pragma once
#include <ncurses.h>
#include <clocale>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include "system_monitor.hpp"

// ─── Color Pair IDs ──────────────────────────────────────────────────────────
#define CP_HEADER   1   // header / shortcut bars  (black on cyan)
#define CP_SELECTED 2   // selected row            (black on white)
#define CP_HIGH     3   // HIGH priority           (red   on default)
#define CP_MEDIUM   4   // MEDIUM priority         (yellow on default)
#define CP_LOW      5   // LOW priority            (green  on default)
#define CP_DONE     6   // DONE status             (green  on default)
#define CP_NORMAL   7   // ordinary text           (white  on default)
#define CP_INPROG   8   // IN PROGRESS status      (yellow on default)
#define CP_STATS    9   // stats-bar labels        (cyan   on default)
#define CP_KEY      10  // shortcut key glyph      (white on cyan)
#define CP_TITLE    11  // column header row       (cyan   on default)
#define CP_DIALOG   12  // dialog background       (white  on default)

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

// ─── Formatting Helpers ───────────────────────────────────────────────────────
inline std::string currentTime() {
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y  %H:%M:%S", &ltm);
    return buf;
}

inline const char* priStr(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "HIGH    ";
        case Priority::MEDIUM: return "MEDIUM  ";
        case Priority::LOW:    return "LOW     ";
    }
    return "        ";
}

inline int priColor(Priority p) {
    switch (p) {
        case Priority::HIGH:   return CP_HIGH;
        case Priority::MEDIUM: return CP_MEDIUM;
        case Priority::LOW:    return CP_LOW;
    }
    return CP_NORMAL;
}

inline const char* statusStr(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return "PENDING   ";
        case TaskStatus::IN_PROGRESS: return "IN PROG   ";
        case TaskStatus::DONE:        return "DONE      ";
    }
    return "          ";
}

inline int statusColor(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return CP_NORMAL;
        case TaskStatus::IN_PROGRESS: return CP_INPROG;
        case TaskStatus::DONE:        return CP_DONE;
    }
    return CP_NORMAL;
}

// ─── Column Layout ────────────────────────────────────────────────────────────
inline int nameWidth(int cols) {
    return std::max(10, cols - 47);
}

// ─── Dialog: Add Task ─────────────────────────────────────────────────────────
inline bool dialogAdd(std::string& outName, Priority& outPrio) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int dh = 12, dw = 54;
    WINDOW* w = newwin(dh, dw, (rows - dh) / 2, (cols - dw) / 2);
    wbkgd(w, COLOR_PAIR(CP_DIALOG));
    box(w, 0, 0);

    // Title bar
    wattron(w, COLOR_PAIR(CP_HEADER) | A_BOLD);
    const char* title = " Add New Task ";
    mvwprintw(w, 0, (dw - static_cast<int>(strlen(title))) / 2, "%s", title);
    wattroff(w, COLOR_PAIR(CP_HEADER) | A_BOLD);

    // Labels
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(w, 2, 3, "Task Name:");
    mvwprintw(w, 6, 3, "Priority:");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD);

    // Input field
    wattron(w, COLOR_PAIR(CP_SELECTED));
    for (int x = 3; x < dw - 3; x++) mvwaddch(w, 4, x, ' ');
    wattroff(w, COLOR_PAIR(CP_SELECTED));

    // Priority buttons
    wattron(w, COLOR_PAIR(CP_LOW)    | A_BOLD); mvwprintw(w, 7, 5,  " [L] Low    "); wattroff(w, COLOR_PAIR(CP_LOW)    | A_BOLD);
    wattron(w, COLOR_PAIR(CP_MEDIUM) | A_BOLD); mvwprintw(w, 7, 18, " [M] Medium "); wattroff(w, COLOR_PAIR(CP_MEDIUM) | A_BOLD);
    wattron(w, COLOR_PAIR(CP_HIGH)   | A_BOLD); mvwprintw(w, 7, 31, " [H] High   "); wattroff(w, COLOR_PAIR(CP_HIGH)   | A_BOLD);

    mvwprintw(w, 10, 3, "[ENTER] Accept Medium    [ESC] Cancel");
    wrefresh(w);

    // Collect task name
    echo();
    curs_set(1);
    char buf[48] = {};
    wmove(w, 4, 3);
    wattron(w, COLOR_PAIR(CP_SELECTED));
    wgetnstr(w, buf, dw - 8);
    wattroff(w, COLOR_PAIR(CP_SELECTED));
    noecho();
    curs_set(0);

    outName = buf;
    // Trim trailing whitespace
    while (!outName.empty() && outName.back() == ' ')
        outName.pop_back();

    if (outName.empty()) {
        delwin(w);
        touchwin(stdscr);
        return false;
    }

    // Collect priority key
    outPrio = Priority::MEDIUM;
    while (true) {
        int ch = wgetch(w);
        if (ch == 'l' || ch == 'L') { outPrio = Priority::LOW;    break; }
        if (ch == 'm' || ch == 'M') { outPrio = Priority::MEDIUM; break; }
        if (ch == 'h' || ch == 'H') { outPrio = Priority::HIGH;   break; }
        if (ch == '\n' || ch == KEY_ENTER) break;   // accept default MEDIUM
        if (ch == 27) {                              // ESC → cancel
            delwin(w);
            touchwin(stdscr);
            return false;
        }
    }

    delwin(w);
    touchwin(stdscr);
    return true;
}

// ─── Dialog: Confirm Delete ───────────────────────────────────────────────────
inline bool dialogConfirmDelete(const std::string& name) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int dh = 8, dw = 56;
    WINDOW* w = newwin(dh, dw, (rows - dh) / 2, (cols - dw) / 2);
    wbkgd(w, COLOR_PAIR(CP_DIALOG));
    box(w, 0, 0);

    wattron(w, COLOR_PAIR(CP_HIGH) | A_BOLD);
    const char* title = " Confirm Delete ";
    mvwprintw(w, 0, (dw - static_cast<int>(strlen(title))) / 2, "%s", title);
    wattroff(w, COLOR_PAIR(CP_HIGH) | A_BOLD);

    std::string msg = "Delete: \"" + name + "\"?";
    if (static_cast<int>(msg.size()) > dw - 4)
        msg = "Delete: \"" + name.substr(0, dw - 18) + "...\"?";
    mvwprintw(w, 3, (dw - static_cast<int>(msg.size())) / 2, "%s", msg.c_str());

    wattron(w, COLOR_PAIR(CP_LOW)  | A_BOLD); mvwprintw(w, 6, 6,  " [Y] Yes, delete "); wattroff(w, COLOR_PAIR(CP_LOW)  | A_BOLD);
    wattron(w, COLOR_PAIR(CP_HIGH) | A_BOLD); mvwprintw(w, 6, 32, " [N] Cancel ");      wattroff(w, COLOR_PAIR(CP_HIGH) | A_BOLD);

    wrefresh(w);
    int ch = wgetch(w);
    delwin(w);
    touchwin(stdscr);
    return (ch == 'y' || ch == 'Y');
}

// ─── Progress Bar ─────────────────────────────────────────────────────────────
inline void drawProgressBar(int y, int x, int width, int percent, int colorPair) {
    int filled = percent * width / 100;
    attron(COLOR_PAIR(colorPair) | A_BOLD);
    for (int i = 0; i < width; i++)
        mvaddch(y, x + i, (i < filled) ? ACS_BLOCK : ACS_HLINE);
    attroff(COLOR_PAIR(colorPair) | A_BOLD);
}

// ─── System Dashboard ─────────────────────────────────────────────────────────
inline void drawSystemDashboard(const SystemMonitor& sysmon) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (rows < 8 || cols < 55) {
        clear();
        mvprintw(0, 0, "Terminal too small! Minimum: 55 columns x 8 rows.");
        refresh();
        return;
    }

    erase();

    // ── Header bar ────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (int x = 0; x < cols; x++) mvaddch(0, x, ' ');
    mvprintw(0, 2, " C++ TASK MANAGER  v1.0  -  System Dashboard");
    std::string dt = currentTime();
    if (static_cast<int>(dt.size()) + 4 < cols)
        mvprintw(0, cols - static_cast<int>(dt.size()) - 2, "%s", dt.c_str());
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    // ── Section header ────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvhline(1, 0, ACS_HLINE, cols);
    mvprintw(1, 2, " SYSTEM MONITOR ");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
    attron(COLOR_PAIR(CP_TITLE));
    mvhline(2, 0, ACS_HLINE, cols);
    attroff(COLOR_PAIR(CP_TITLE));

    const int barW   = std::min(50, cols - 22);
    const int labelX = std::max(2, (cols - barW - 18) / 2);
    const int barX   = labelX + 18;

    // ── CPU ───────────────────────────────────────────────────────────────────
    int cpuPct   = static_cast<int>(sysmon.cpuUsage);
    int cpuColor = (cpuPct > 80) ? CP_HIGH : (cpuPct > 50) ? CP_MEDIUM : CP_LOW;

    attron(COLOR_PAIR(CP_STATS) | A_BOLD);
    mvprintw(4, labelX, "CPU Usage:  %3d%%  ", cpuPct);
    attroff(COLOR_PAIR(CP_STATS) | A_BOLD);
    drawProgressBar(4, barX, barW, cpuPct, cpuColor);

    // ── RAM ───────────────────────────────────────────────────────────────────
    int memPct   = sysmon.mem.usedPercent();
    int memColor = (memPct > 80) ? CP_HIGH : (memPct > 50) ? CP_MEDIUM : CP_LOW;

    long long usedMB  = sysmon.mem.used()  / 1024;
    long long totalMB = sysmon.mem.total   / 1024;

    attron(COLOR_PAIR(CP_STATS) | A_BOLD);
    mvprintw(7, labelX, "RAM Usage:  %3d%%  ", memPct);
    attroff(COLOR_PAIR(CP_STATS) | A_BOLD);
    drawProgressBar(7, barX, barW, memPct, memColor);

    attron(COLOR_PAIR(CP_NORMAL));
    mvprintw(8, barX, "%lld MB used / %lld MB total",
             usedMB, totalMB);
    attroff(COLOR_PAIR(CP_NORMAL));

    // ── Bottom separator ──────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_TITLE));
    mvhline(rows - 3, 0, ACS_HLINE, cols);
    attroff(COLOR_PAIR(CP_TITLE));

    // ── Stats placeholder ─────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_NORMAL));
    for (int x = 0; x < cols; x++) mvaddch(rows - 2, x, ' ');
    attroff(COLOR_PAIR(CP_NORMAL));
    attron(COLOR_PAIR(CP_STATS) | A_BOLD);
    mvprintw(rows - 2, 1, "Live metrics refreshed every 0.5 s");
    attroff(COLOR_PAIR(CP_STATS) | A_BOLD);

    // ── Shortcut bar ──────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_HEADER));
    for (int x = 0; x < cols; x++) mvaddch(rows - 1, x, ' ');
    attroff(COLOR_PAIR(CP_HEADER));

    struct { const char* key; const char* desc; } sc[] = {
        {"[s]", "Tasks  "},
        {"[q]", "Quit"}
    };
    int sx = 1;
    for (auto& s : sc) {
        if (sx >= cols - 12) break;
        attron(COLOR_PAIR(CP_KEY) | A_BOLD);
        mvprintw(rows - 1, sx, "%s", s.key);
        sx += static_cast<int>(strlen(s.key));
        attroff(COLOR_PAIR(CP_KEY) | A_BOLD);
        attron(COLOR_PAIR(CP_HEADER));
        mvprintw(rows - 1, sx, "%s", s.desc);
        sx += static_cast<int>(strlen(s.desc));
        attroff(COLOR_PAIR(CP_HEADER));
    }

    refresh();
}

// ─── Main UI Renderer ────────────────────────────────────────────────────────
inline void drawUI(const TaskManager& tm, int sel, int scroll,
                   const std::string& statusMsg)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (rows < 8 || cols < 55) {
        clear();
        mvprintw(0, 0, "Terminal too small! Minimum: 55 columns x 8 rows.");
        refresh();
        return;
    }

    erase();

    const int nW = nameWidth(cols);

    // ── Header bar ────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (int x = 0; x < cols; x++) mvaddch(0, x, ' ');
    mvprintw(0, 2, " C++ TASK MANAGER  v1.0");
    std::string dt = currentTime();
    if (static_cast<int>(dt.size()) + 4 < cols)
        mvprintw(0, cols - static_cast<int>(dt.size()) - 2, "%s", dt.c_str());
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    // ── Column header ─────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvhline(1, 0, ACS_HLINE, cols);
    mvprintw(1, 0, "  %-4s | %-8s | %-*s | %-10s | %-10s",
             "ID", "PRIORITY", nW, "TASK NAME", "STATUS", "CREATED");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    attron(COLOR_PAIR(CP_TITLE));
    mvhline(2, 0, ACS_HLINE, cols);
    attroff(COLOR_PAIR(CP_TITLE));

    // ── Task rows ─────────────────────────────────────────────────────────────
    const int listH  = rows - 6;
    const int nTasks = static_cast<int>(tm.tasks.size());

    for (int i = 0; i < listH; i++) {
        const int idx = i + scroll;
        const int y   = 3 + i;

        if (idx >= nTasks) {
            // blank row
            attron(COLOR_PAIR(CP_NORMAL));
            for (int x = 0; x < cols; x++) mvaddch(y, x, ' ');
            attroff(COLOR_PAIR(CP_NORMAL));
            continue;
        }

        const Task& t   = tm.tasks[idx];
        const bool isSel = (idx == sel);

        // Row background
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(CP_NORMAL));
        for (int x = 0; x < cols; x++) mvaddch(y, x, ' ');
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD) : COLOR_PAIR(CP_NORMAL));

        // Selector + ID
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(y, 0, " %c %-4d|", isSel ? '>' : ' ', t.id);
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD) : COLOR_PAIR(CP_NORMAL));

        // Priority
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(priColor(t.priority)) | A_BOLD);
        mvprintw(y, 8, " %-8s", priStr(t.priority));
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD)
                      : (COLOR_PAIR(priColor(t.priority)) | A_BOLD));

        // Separator
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(y, 17, "|");
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD) : COLOR_PAIR(CP_NORMAL));

        // Task name (truncated if needed)
        std::string name = t.name;
        if (static_cast<int>(name.size()) > nW)
            name = name.substr(0, nW - 3) + "...";

        if (isSel)
            attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (t.status == TaskStatus::DONE)
            attron(COLOR_PAIR(CP_DONE) | A_DIM);
        else
            attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(y, 18, " %-*s ", nW, name.c_str());
        if (isSel)
            attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (t.status == TaskStatus::DONE)
            attroff(COLOR_PAIR(CP_DONE) | A_DIM);
        else
            attroff(COLOR_PAIR(CP_NORMAL));

        const int sepX = 20 + nW;

        // Status
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(y, sepX, "|");
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD) : COLOR_PAIR(CP_NORMAL));

        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(statusColor(t.status)) | A_BOLD);
        mvprintw(y, sepX + 1, " %-10s", statusStr(t.status));
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD)
                      : (COLOR_PAIR(statusColor(t.status)) | A_BOLD));

        // Created date
        if (isSel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(y, sepX + 12, "| %-10s", t.created.c_str());
        attroff(isSel ? (COLOR_PAIR(CP_SELECTED) | A_BOLD) : COLOR_PAIR(CP_NORMAL));
    }

    // Empty-state message
    if (tm.tasks.empty()) {
        attron(COLOR_PAIR(CP_MEDIUM) | A_BOLD);
        const int midY = 3 + listH / 2;
        const char* em1 = "No tasks yet!";
        const char* em2 = "Press  [a]  to add your first task";
        mvprintw(midY - 1, (cols - static_cast<int>(strlen(em1))) / 2, "%s", em1);
        mvprintw(midY,     (cols - static_cast<int>(strlen(em2))) / 2, "%s", em2);
        attroff(COLOR_PAIR(CP_MEDIUM) | A_BOLD);
    }

    // ── Separator ─────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_TITLE));
    mvhline(rows - 3, 0, ACS_HLINE, cols);
    attroff(COLOR_PAIR(CP_TITLE));

    // ── Stats bar ─────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_NORMAL));
    for (int x = 0; x < cols; x++) mvaddch(rows - 2, x, ' ');
    attroff(COLOR_PAIR(CP_NORMAL));

    attron(COLOR_PAIR(CP_STATS)  | A_BOLD); mvprintw(rows - 2,  1, "TASKS: %-3d",  nTasks);                           attroff(COLOR_PAIR(CP_STATS)  | A_BOLD);
    attron(COLOR_PAIR(CP_NORMAL) | A_BOLD); mvprintw(rows - 2, 13, "Pending: %-3d", tm.count(TaskStatus::PENDING));   attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    attron(COLOR_PAIR(CP_INPROG) | A_BOLD); mvprintw(rows - 2, 27, "In Progress: %-3d", tm.count(TaskStatus::IN_PROGRESS)); attroff(COLOR_PAIR(CP_INPROG) | A_BOLD);
    attron(COLOR_PAIR(CP_DONE)   | A_BOLD); mvprintw(rows - 2, 47, "Done: %-3d", tm.count(TaskStatus::DONE));         attroff(COLOR_PAIR(CP_DONE)   | A_BOLD);

    if (!statusMsg.empty()) {
        attron(COLOR_PAIR(CP_LOW) | A_BOLD);
        mvprintw(rows - 2, cols - static_cast<int>(statusMsg.size()) - 2,
                 "%s", statusMsg.c_str());
        attroff(COLOR_PAIR(CP_LOW) | A_BOLD);
    }

    // ── Shortcut bar ──────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_HEADER));
    for (int x = 0; x < cols; x++) mvaddch(rows - 1, x, ' ');
    attroff(COLOR_PAIR(CP_HEADER));

    struct { const char* key; const char* desc; } shortcuts[] = {
        {"[a]", "Add "},
        {"[d]", "Delete "},
        {"[Enter]", "Toggle Status "},
        {"[p]", "Priority "},
        {"[j/k]", "Navigate "},
        {"[s]", "System "},
        {"[q]", "Quit"}
    };

    int sx = 1;
    for (auto& s : shortcuts) {
        if (sx >= cols - 12) break;
        attron(COLOR_PAIR(CP_KEY) | A_BOLD);
        mvprintw(rows - 1, sx, "%s", s.key);
        sx += static_cast<int>(strlen(s.key));
        attroff(COLOR_PAIR(CP_KEY) | A_BOLD);

        attron(COLOR_PAIR(CP_HEADER));
        mvprintw(rows - 1, sx, "%s", s.desc);
        sx += static_cast<int>(strlen(s.desc));
        attroff(COLOR_PAIR(CP_HEADER));
    }

    refresh();
}
