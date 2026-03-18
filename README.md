# C++ Task Manager (TUI)

A professional terminal-based task manager built with C++ and ncurses.

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
./build/task-manager
```

> Minimum terminal size: **55 columns x 8 rows**

## REST API Server

A Node.js REST API server is included in the `server/` directory.

### Setup

```bash
cd server
npm install
node server.js
```

The server starts on `http://localhost:3000` by default (override with the `PORT` environment variable).

### Endpoints

| Method   | Path         | Description                          |
|----------|--------------|--------------------------------------|
| `GET`    | `/tasks`     | List all tasks                       |
| `POST`   | `/tasks`     | Create a task `{ name, priority? }`  |
| `GET`    | `/tasks/:id` | Get a single task                    |
| `PATCH`  | `/tasks/:id` | Update `name`, `priority`, `status`  |
| `DELETE` | `/tasks/:id` | Delete a task                        |
| `GET`    | `/stats`     | Task counts by status                |

**Priority values:** `LOW` · `MEDIUM` · `HIGH`  
**Status values:** `PENDING` · `IN_PROGRESS` · `DONE`
