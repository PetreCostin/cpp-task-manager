# CPP Distributed Dashboard

A distributed system-monitoring dashboard consisting of three components:

```
cpp-distributed-dashboard/
├── agent/                # C++ system monitor
│   ├── main.cpp
│   ├── system_monitor.cpp
│   └── system_monitor.hpp
│
├── server/               # Node.js API
│   ├── server.js
│   └── package.json
│
└── web/                  # React dashboard
    ├── src/
    │   ├── App.jsx
    │   └── Dashboard.jsx
    └── package.json
```

## How it works

1. **C++ agent** – collects CPU usage, memory stats, uptime, and top processes
   from `/proc` every 2 seconds and HTTP-POSTs a JSON snapshot to the Node.js
   server.
2. **Node.js server** – stores the latest snapshot and broadcasts it over
   WebSocket. Also exposes REST endpoints (`GET /api/metrics`,
   `GET /api/metrics/history`).
3. **React web dashboard** – connects via WebSocket (falling back to HTTP
   polling) and displays live gauges, metric cards, and a process table.

## Quick start

### 1 – Build the C++ agent

Requires **CMake ≥ 3.16** and a **C++17** compiler (Linux only – reads `/proc`).

```bash
mkdir build && cd build
cmake ..
make
# produces build/agent/agent
```

### 2 – Start the Node.js server

```bash
cd server
npm install
npm start
# listening on http://localhost:3001
```

### 3 – Start the React web dashboard

```bash
cd web
npm install
npm run dev
# open http://localhost:5173
```

### 4 – Run the C++ agent

```bash
./build/agent/agent
# optional flags: --host localhost --port 3001 --interval 2000
```

## REST API

| Method | Path                   | Description                          |
|--------|------------------------|--------------------------------------|
| `POST` | `/api/metrics`         | Ingest a JSON snapshot (C++ agent)   |
| `GET`  | `/api/metrics`         | Latest snapshot                      |
| `GET`  | `/api/metrics/history` | Last 60 snapshots (≈ 2 min)         |

WebSocket upgrades are also served on port **3001**.
