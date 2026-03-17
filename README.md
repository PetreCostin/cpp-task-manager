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

---

## Metrics API Server

A lightweight Node.js/Express server that receives and serves system metrics over HTTP.

### Setup

Requires **Node.js** (v14+).

```bash
cd server
npm install
npm start
```

The server listens on **port 3000** by default (override with `PORT` env var).

### Endpoints

| Method | Path       | Description                              |
|--------|------------|------------------------------------------|
| `POST` | `/metrics` | Submit metrics as a JSON body            |
| `GET`  | `/metrics` | Retrieve the most recently posted metrics |

### Example

```bash
# Post metrics
curl -X POST http://localhost:3000/metrics \
  -H "Content-Type: application/json" \
  -d '{"cpu": 42, "memory": 67}'

# Get latest metrics
curl http://localhost:3000/metrics
# {"cpu":42,"memory":67}
```
