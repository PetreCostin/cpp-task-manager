# 🖥️ C++ System Dashboard (TUI)

A real-time terminal dashboard built in C++ using ncurses, combining live system
monitoring with a full-featured task manager.

## 🚀 Features

- 📊 **Live system stats** — CPU usage bar, memory usage bar, system uptime
- 📝 **Task manager** — add / delete / cycle status / cycle priority
- 🕒 **Real-time clock** — updates every 0.5 s in the header
- 🧩 **Multi-window layout** — dedicated ncurses panels for system stats and tasks
- ⚡ **Event-driven UI loop** — `halfdelay` keeps the display live while staying responsive to input

## 🧠 Concepts Demonstrated

- Low-level terminal UI (ncurses `WINDOW*` panels)
- Reading Linux `/proc/stat`, `/proc/meminfo`, `/proc/uptime`
- Event-driven architecture with `halfdelay`
- State management (task lifecycle, scroll position)
- Modular C++ design (single-file, clearly sectioned)
- Real-time rendering with color-coded progress bars

## 🖼️ Layout

```
┌─ C++ SYSTEM DASHBOARD v2.0 ──────────────── 17 Mar 2026  11:09:12 ─┐
├─ SYSTEM STATS ──────────────────────────────────────────────────────┤
│ CPU [████████████░░░░░░░░]  62%                                     │
│ MEM [████████░░░░░░░░░░░░]  41%  3.3G / 8.0G                       │
│ Uptime: 1d 02:15:42                                                 │
├─ TASK MANAGER ──────────────────────────────────────────────────────┤
│   ID | PRIORITY | TASK NAME            | STATUS     | CREATED       │
│──────────────────────────────────────────────────────────────────── │
│ > 1  | HIGH     | Monitor CPU & memory | PENDING    | 03/17/2026    │
│   2  | HIGH     | Multi-window layout  | DONE       | 03/17/2026    │
│   …                                                                 │
├─────────────────────────────────────────────────────────────────────┤
│ TASKS: 5   Pending: 3   In Progress: 1   Done: 1                    │
│ [a] Add  [d] Delete  [Enter] Toggle Status  [p] Priority  [q] Quit  │
└─────────────────────────────────────────────────────────────────────┘
```

## Controls

| Key | Action |
|-----|--------|
| `↑` / `k` | Navigate up |
| `↓` / `j` | Navigate down |
| `PgUp` / `PgDn` | Page up / down |
| `Home` / `End` | First / last task |
| `a` | Add a new task |
| `d` | Delete selected task (with confirmation) |
| `Enter` | Cycle task status (Pending → In Progress → Done) |
| `p` | Cycle task priority (Low → Medium → High) |
| `q` | Quit |

## Dependencies

### Install ncurses

**Ubuntu / Debian:**
```bash
sudo apt install libncurses5-dev libncursesw5-dev
```

**macOS (Homebrew):**
```bash
brew install ncurses
```

## Build

Requires **CMake >= 3.16** and a **C++17** compiler.

```bash
mkdir build && cd build
cmake ..
make
```

## ▶️ Run

```bash
./build/cpp-system-dashboard
```

> Minimum terminal size: **60 columns × 14 rows**
