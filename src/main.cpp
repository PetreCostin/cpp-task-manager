#include <ncurses.h>
#include <clocale>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <cstdio>

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
#define CP_BAR_OK   13  // progress bar – low usage    (green  on default)
#define CP_BAR_WARN 14  // progress bar – medium usage (yellow on default)
#define CP_BAR_CRIT 15  // progress bar – high usage   (red    on default)

// ─── Layout Constants ─────────────────────────────────────────────────────────
static const int HEADER_H = 1;   // top header bar
static const int STATS_H  = 5;   // system-stats panel  (border + 3 data rows + border)
static const int FOOTER_H = 2;   // stats-summary bar + shortcut bar

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

// ─── System Monitoring ───────────────────────────────────────────────────────
struct CpuStat {
    long user = 0, nice = 0, system = 0, idle = 0;
    long iowait = 0, irq = 0, softirq = 0, steal = 0;
};

struct MemInfo {
    long totalKB = 0, availKB = 0;
};

static bool readCpuStat(CpuStat& s) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return false;
    int n = fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
                   &s.user, &s.nice, &s.system, &s.idle,
                   &s.iowait, &s.irq, &s.softirq, &s.steal);
    fclose(f);
    return n == 8;
}

static int calcCpuPercent(const CpuStat& prev, const CpuStat& cur) {
    long prevIdle    = prev.idle + prev.iowait;
    long curIdle     = cur.idle  + cur.iowait;
    long prevNonIdle = prev.user + prev.nice + prev.system
                     + prev.irq + prev.softirq + prev.steal;
    long curNonIdle  = cur.user  + cur.nice  + cur.system
                     + cur.irq  + cur.softirq  + cur.steal;
    long totalDelta  = (curIdle + curNonIdle) - (prevIdle + prevNonIdle);
    long idleDelta   = curIdle - prevIdle;
    if (totalDelta <= 0) return 0;
    return static_cast<int>(100 * (totalDelta - idleDelta) / totalDelta);
}

static bool readMemInfo(MemInfo& m) {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return false;
    char line[256];
    m.totalKB = m.availKB = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0)
            sscanf(line, "MemTotal: %ld kB", &m.totalKB);
        else if (strncmp(line, "MemAvailable:", 13) == 0)
            sscanf(line, "MemAvailable: %ld kB", &m.availKB);
    }
    fclose(f);
    return m.totalKB > 0;
}

static std::string fmtBytes(long kb) {
    char buf[32];
    if (kb >= 1024L * 1024L)
        snprintf(buf, sizeof(buf), "%.1fG", kb / (1024.0 * 1024.0));
    else if (kb >= 1024L)
        snprintf(buf, sizeof(buf), "%.1fM", kb / 1024.0);
    else
        snprintf(buf, sizeof(buf), "%ldK", kb);
    return buf;
}

static std::string readUptime() {
    FILE* f = fopen("/proc/uptime", "r");
    if (!f) return "N/A";
    double upSec = 0.0;
    if (fscanf(f, "%lf", &upSec) != 1) upSec = 0.0;
    fclose(f);
    long sec  = static_cast<long>(upSec);
    long days = sec / 86400; sec %= 86400;
    long hrs  = sec / 3600;  sec %= 3600;
    long mins = sec / 60;    sec %= 60;
    char buf[32];
    if (days > 0)
        snprintf(buf, sizeof(buf), "%ldd %02ld:%02ld:%02ld", days, hrs, mins, sec);
    else
        snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", hrs, mins, sec);
    return buf;
}

// ─── Formatting Helpers ───────────────────────────────────────────────────────
static std::string currentTime() {
    time_t now = time(nullptr);
    struct tm ltm {};
    localtime_r(&now, &ltm);
    char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y  %H:%M:%S", &ltm);
    return buf;
}

static const char* priStr(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "HIGH    ";
        case Priority::MEDIUM: return "MEDIUM  ";
        case Priority::LOW:    return "LOW     ";
    }
    return "        ";
}

static int priColor(Priority p) {
    switch (p) {
        case Priority::HIGH:   return CP_HIGH;
        case Priority::MEDIUM: return CP_MEDIUM;
        case Priority::LOW:    return CP_LOW;
    }
    return CP_NORMAL;
}

static const char* statusStr(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return "PENDING   ";
        case TaskStatus::IN_PROGRESS: return "IN PROG   ";
        case TaskStatus::DONE:        return "DONE      ";
    }
    return "          ";
}

static int statusColor(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return CP_NORMAL;
        case TaskStatus::IN_PROGRESS: return CP_INPROG;
        case TaskStatus::DONE:        return CP_DONE;
    }
    return CP_NORMAL;
}

// ─── Column Layout ────────────────────────────────────────────────────────────
//  Pos  0 : ' '
//  Pos  1 : selector ('>' or ' ')
//  Pos  2 : ' '
//  Pos  3 : ID   (4 chars)
//  Pos  7 : ' | '
//  Pos 10 : Priority (8 chars)
//  Pos 18 : ' | '
//  Pos 21 : Name (nameW chars)
//  Pos 21+nameW : ' | '
//  Pos 24+nameW : Status (10 chars)
//  Pos 34+nameW : ' | '
//  Pos 37+nameW : Created (10 chars)
//  Total = 47 + nameW  =>  nameW = cols - 47
static int nameWidth(int cols) {
    return std::max(10, cols - 47);
}

// ─── Progress Bar ────────────────────────────────────────────────────────────
// Draws a progress bar of `barW` chars at (y,x) inside window `w`.
// Uses color-coded fill blocks: green < 60 %, yellow < 85 %, red >= 85 %.
static void drawProgressBar(WINDOW* w, int y, int x, int barW, int pct) {
    pct = std::max(0, std::min(100, pct));
    int cp     = (pct < 60) ? CP_BAR_OK : (pct < 85) ? CP_BAR_WARN : CP_BAR_CRIT;
    int filled = barW * pct / 100;

    wattron(w, COLOR_PAIR(cp) | A_BOLD);
    for (int i = 0; i < filled; i++)
        mvwaddch(w, y, x + i, ACS_BLOCK);
    wattroff(w, COLOR_PAIR(cp) | A_BOLD);

    wattron(w, COLOR_PAIR(CP_NORMAL));
    for (int i = filled; i < barW; i++)
        mvwaddch(w, y, x + i, ACS_CKBOARD);
    wattroff(w, COLOR_PAIR(CP_NORMAL));
}

// ─── Stats Panel ──────────────────────────────────────────────────────────────
// Renders the system-stats WINDOW* (CPU, Memory, Uptime).
static void drawStatsPanel(WINDOW* win, int cpuPct, const MemInfo& mem) {
    int wrows, wcols;
    getmaxyx(win, wrows, wcols);
    (void)wrows;

    werase(win);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));
    box(win, 0, 0);

    // Panel title
    wattron(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvwprintw(win, 0, 2, " SYSTEM STATS ");
    wattroff(win, COLOR_PAIR(CP_HEADER) | A_BOLD);

    const int barW = std::max(10, wcols / 2 - 16);

    // ── CPU row ───────────────────────────────────────────────────────────────
    wattron(win, COLOR_PAIR(CP_STATS) | A_BOLD);
    mvwprintw(win, 1, 2, "CPU");
    wattroff(win, COLOR_PAIR(CP_STATS) | A_BOLD);

    wattron(win, COLOR_PAIR(CP_NORMAL));
    mvwaddch(win, 1, 6, '[');
    mvwaddch(win, 1, 7 + barW, ']');
    wattroff(win, COLOR_PAIR(CP_NORMAL));

    drawProgressBar(win, 1, 7, barW, cpuPct);

    wattron(win, COLOR_PAIR(CP_STATS) | A_BOLD);
    mvwprintw(win, 1, 9 + barW, "%3d%%", cpuPct);
    wattroff(win, COLOR_PAIR(CP_STATS) | A_BOLD);

    // ── Memory row ────────────────────────────────────────────────────────────
    int memPct = (mem.totalKB > 0)
        ? static_cast<int>(100L * (mem.totalKB - mem.availKB) / mem.totalKB)
        : 0;

    wattron(win, COLOR_PAIR(CP_STATS) | A_BOLD);
    mvwprintw(win, 2, 2, "MEM");
    wattroff(win, COLOR_PAIR(CP_STATS) | A_BOLD);

    wattron(win, COLOR_PAIR(CP_NORMAL));
    mvwaddch(win, 2, 6, '[');
    mvwaddch(win, 2, 7 + barW, ']');
    wattroff(win, COLOR_PAIR(CP_NORMAL));

    drawProgressBar(win, 2, 7, barW, memPct);

    std::string usedStr  = fmtBytes(mem.totalKB - mem.availKB);
    std::string totalStr = fmtBytes(mem.totalKB);
    wattron(win, COLOR_PAIR(CP_STATS) | A_BOLD);
    mvwprintw(win, 2, 9 + barW, "%3d%%  %s / %s",
              memPct, usedStr.c_str(), totalStr.c_str());
    wattroff(win, COLOR_PAIR(CP_STATS) | A_BOLD);

    // ── Uptime row ────────────────────────────────────────────────────────────
    wattron(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(win, 3, 2, "Uptime: %s", readUptime().c_str());
    wattroff(win, COLOR_PAIR(CP_TITLE) | A_BOLD);

    wrefresh(win);
}

// ─── Dialog: Add Task ─────────────────────────────────────────────────────────
static bool dialogAdd(std::string& outName, Priority& outPrio) {
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
static bool dialogConfirmDelete(const std::string& name) {
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

// ─── Task Panel ───────────────────────────────────────────────────────────────
// Renders the task-list WINDOW*: column headers + scrollable task rows.
static void drawTaskPanel(WINDOW* win, const TaskManager& tm,
                          int sel, int scroll)
{
    int wrows, wcols;
    getmaxyx(win, wrows, wcols);

    werase(win);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));

    const int nW = nameWidth(wcols);

    // ── Panel title / top border ──────────────────────────────────────────────
    wattron(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvwhline(win, 0, 0, ' ', wcols);
    mvwprintw(win, 0, 2, " TASK MANAGER ");
    wattroff(win, COLOR_PAIR(CP_HEADER) | A_BOLD);

    // ── Column header ─────────────────────────────────────────────────────────
    wattron(win, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwhline(win, 1, 0, ACS_HLINE, wcols);
    mvwprintw(win, 1, 0, "  %-4s | %-8s | %-*s | %-10s | %-10s",
              "ID", "PRIORITY", nW, "TASK NAME", "STATUS", "CREATED");
    wattroff(win, COLOR_PAIR(CP_TITLE) | A_BOLD);

    wattron(win, COLOR_PAIR(CP_TITLE));
    mvwhline(win, 2, 0, ACS_HLINE, wcols);
    wattroff(win, COLOR_PAIR(CP_TITLE));

    // ── Task rows ─────────────────────────────────────────────────────────────
    const int listH  = wrows - 3;   // rows 3..wrows-1
    const int nTasks = static_cast<int>(tm.tasks.size());

    for (int i = 0; i < listH; i++) {
        const int idx  = i + scroll;
        const int y    = 3 + i;
        const bool isSel = (idx == sel);

        if (idx >= nTasks) {
            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwhline(win, y, 0, ' ', wcols);
            wattroff(win, COLOR_PAIR(CP_NORMAL));
            continue;
        }

        const Task& t = tm.tasks[idx];

        // Row background
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwhline(win, y, 0, ' ', wcols);
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(CP_NORMAL));

        // Selector + ID
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, 0, " %c %-4d|", isSel ? '>' : ' ', t.id);
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(CP_NORMAL));

        // Priority
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(priColor(t.priority)) | A_BOLD);
        mvwprintw(win, y, 8, " %-8s", priStr(t.priority));
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(priColor(t.priority)) | A_BOLD);

        // Separator
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, 17, "|");
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(CP_NORMAL));

        // Task name (truncated if needed)
        std::string name = t.name;
        if (static_cast<int>(name.size()) > nW)
            name = name.substr(0, nW - 3) + "...";

        if (isSel)
            wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (t.status == TaskStatus::DONE)
            wattron(win, COLOR_PAIR(CP_DONE) | A_DIM);
        else
            wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, 18, " %-*s ", nW, name.c_str());
        if (isSel)
            wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (t.status == TaskStatus::DONE)
            wattroff(win, COLOR_PAIR(CP_DONE) | A_DIM);
        else
            wattroff(win, COLOR_PAIR(CP_NORMAL));

        const int sepX = 20 + nW;

        // Status
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, sepX, "|");
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(CP_NORMAL));

        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(statusColor(t.status)) | A_BOLD);
        mvwprintw(win, y, sepX + 1, " %-10s", statusStr(t.status));
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(statusColor(t.status)) | A_BOLD);

        // Created date
        if (isSel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, y, sepX + 12, "| %-10s", t.created.c_str());
        if (isSel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else        wattroff(win, COLOR_PAIR(CP_NORMAL));
    }

    // Empty-state message
    if (tm.tasks.empty()) {
        wattron(win, COLOR_PAIR(CP_MEDIUM) | A_BOLD);
        const int midY = 3 + listH / 2;
        const char* em1 = "No tasks yet!";
        const char* em2 = "Press  [a]  to add your first task";
        mvwprintw(win, midY - 1,
                  (wcols - static_cast<int>(strlen(em1))) / 2, "%s", em1);
        mvwprintw(win, midY,
                  (wcols - static_cast<int>(strlen(em2))) / 2, "%s", em2);
        wattroff(win, COLOR_PAIR(CP_MEDIUM) | A_BOLD);
    }

    wrefresh(win);
}

// ─── Main UI Renderer ────────────────────────────────────────────────────────
// Draws the header and footer on stdscr, then refreshes the two sub-panels.
static void drawUI(WINDOW* statsWin, WINDOW* taskWin,
                   const TaskManager& tm, int sel, int scroll,
                   int cpuPct, const MemInfo& mem,
                   const std::string& statusMsg)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int minRows = HEADER_H + STATS_H + 4 + FOOTER_H;  // need ≥4 task rows
    if (rows < minRows || cols < 60) {
        clear();
        mvprintw(0, 0, "Terminal too small! Minimum: 60 columns x %d rows.", minRows);
        refresh();
        return;
    }

    erase();

    const int nTasks = static_cast<int>(tm.tasks.size());

    // ── Header bar ────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (int x = 0; x < cols; x++) mvaddch(0, x, ' ');
    mvprintw(0, 2, " C++ SYSTEM DASHBOARD  v2.0");
    std::string dt = currentTime();
    if (static_cast<int>(dt.size()) + 4 < cols)
        mvprintw(0, cols - static_cast<int>(dt.size()) - 2, "%s", dt.c_str());
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    refresh();   // flush header before sub-windows paint over it

    // ── System-stats panel ────────────────────────────────────────────────────
    drawStatsPanel(statsWin, cpuPct, mem);

    // ── Task panel ────────────────────────────────────────────────────────────
    drawTaskPanel(taskWin, tm, sel, scroll);

    // ── Separator ─────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_TITLE));
    mvhline(rows - FOOTER_H - 1, 0, ACS_HLINE, cols);
    attroff(COLOR_PAIR(CP_TITLE));

    // ── Summary stats bar ─────────────────────────────────────────────────────
    attron(COLOR_PAIR(CP_NORMAL));
    for (int x = 0; x < cols; x++) mvaddch(rows - 2, x, ' ');
    attroff(COLOR_PAIR(CP_NORMAL));

    attron(COLOR_PAIR(CP_STATS)  | A_BOLD); mvprintw(rows - 2,  1, "TASKS: %-3d",  nTasks);                                attroff(COLOR_PAIR(CP_STATS)  | A_BOLD);
    attron(COLOR_PAIR(CP_NORMAL) | A_BOLD); mvprintw(rows - 2, 13, "Pending: %-3d", tm.count(TaskStatus::PENDING));        attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    attron(COLOR_PAIR(CP_INPROG) | A_BOLD); mvprintw(rows - 2, 27, "In Progress: %-3d", tm.count(TaskStatus::IN_PROGRESS));attroff(COLOR_PAIR(CP_INPROG) | A_BOLD);
    attron(COLOR_PAIR(CP_DONE)   | A_BOLD); mvprintw(rows - 2, 47, "Done: %-3d", tm.count(TaskStatus::DONE));              attroff(COLOR_PAIR(CP_DONE)   | A_BOLD);

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

// ─── Window helpers ───────────────────────────────────────────────────────────
static WINDOW* makeStatsWin(int cols) {
    return newwin(STATS_H, cols, HEADER_H, 0);
}
static WINDOW* makeTaskWin(int rows, int cols) {
    int h = rows - HEADER_H - STATS_H - FOOTER_H;
    return newwin(std::max(4, h), cols, HEADER_H + STATS_H, 0);
}

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
    init_pair(CP_BAR_OK,   COLOR_GREEN,   -1);
    init_pair(CP_BAR_WARN, COLOR_YELLOW,  -1);
    init_pair(CP_BAR_CRIT, COLOR_RED,     -1);

    // Create sub-windows (panels)
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    WINDOW* statsWin = makeStatsWin(cols);
    WINDOW* taskWin  = makeTaskWin(rows, cols);

    // Pre-populate with sample tasks
    TaskManager tm;
    tm.add("Monitor CPU & memory in real time",  Priority::HIGH);
    tm.add("Implement ncurses multi-window layout", Priority::HIGH);
    tm.add("Add task creation dialog",           Priority::MEDIUM);
    tm.add("Write unit tests for TaskManager",   Priority::MEDIUM);
    tm.add("Update project documentation",       Priority::LOW);
    tm.tasks[1].status = TaskStatus::DONE;
    tm.tasks[2].status = TaskStatus::IN_PROGRESS;

    int sel        = 0;
    int scroll     = 0;
    std::string statusMsg;
    int statusTick = 0;
    const int STATUS_DURATION = 20;

    // System-monitoring state
    CpuStat prevCpu{}, curCpu{};
    MemInfo mem{};
    int cpuPct = 0;
    readCpuStat(prevCpu);
    readMemInfo(mem);

    halfdelay(5);   // 0.5 s refresh for real-time clock and system stats

    while (true) {
        getmaxyx(stdscr, rows, cols);

        // Recompute task-panel visible height
        const int taskWinH = rows - HEADER_H - STATS_H - FOOTER_H;
        const int listH    = std::max(1, taskWinH - 3);  // subtract header rows inside panel
        const int nTasks   = static_cast<int>(tm.tasks.size());

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

        // Refresh system stats on every frame (cheap reads)
        if (readCpuStat(curCpu)) {
            cpuPct  = calcCpuPercent(prevCpu, curCpu);
            prevCpu = curCpu;
        }
        readMemInfo(mem);

        drawUI(statsWin, taskWin, tm, sel, scroll, cpuPct, mem, statusMsg);

        int ch = getch();
        if (ch == ERR) continue;   // halfdelay timeout → just redraw

        switch (ch) {
            case 'q': case 'Q':
                delwin(taskWin);
                delwin(statsWin);
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
                getmaxyx(stdscr, rows, cols);
                delwin(taskWin);
                delwin(statsWin);
                statsWin = makeStatsWin(cols);
                taskWin  = makeTaskWin(rows, cols);
                clear();
                break;

            default:
                break;
        }
    }

    delwin(taskWin);
    delwin(statsWin);
    endwin();
    return 0;
}
