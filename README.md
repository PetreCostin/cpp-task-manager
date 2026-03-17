# C++ System Dashboard Pro (TUI)

A professional terminal-based system dashboard and task manager built with C++ and ncurses.

## Project Structure

```
cpp-system-dashboard-pro/
├── README.md
├── CMakeLists.txt
└── src/
    ├── main.cpp
    ├── system_monitor.cpp
    ├── system_monitor.hpp
    ├── task_manager.cpp
    ├── task_manager.hpp
    ├── ui.cpp
    └── ui.hpp
```

## Features

- Color-coded priorities: **HIGH** (red) / **MEDIUM** (yellow) / **LOW** (green)
- Task status lifecycle: **Pending → In Progress → Done**
- Real-time clock in header
- Scrollable task list with selection indicator
- Add-task dialog with inline name entry and priority picker
- Delete confirmation dialog
- Stats bar (total / pending / in-progress / done)
- Keyboard shortcut bar
- Terminal resize support

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

## Run

```bash
./build/cpp-system-dashboard-pro
```

> Minimum terminal size: **55 columns x 8 rows**
