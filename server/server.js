'use strict';

const express    = require('express');
const cors       = require('cors');
const http       = require('http');
const { WebSocketServer } = require('ws');

const app    = express();
const server = http.createServer(app);
const wss    = new WebSocketServer({ server });

const PORT   = process.env.PORT || 3001;

// ─── Middleware ───────────────────────────────────────────────────────────────
app.use(cors());
app.use(express.json());

// ─── In-memory store ─────────────────────────────────────────────────────────
let latestMetrics = null;
const MAX_HISTORY = 60;          // keep last 60 snapshots (~2 min at 2 s each)
const metricsHistory = [];

// ─── REST: receive metrics from the C++ agent ─────────────────────────────────
app.post('/api/metrics', (req, res) => {
    const body = req.body;
    if (!body || typeof body.cpuUsagePercent !== 'number') {
        return res.status(400).json({ error: 'Invalid metrics payload' });
    }

    latestMetrics = { ...body, timestamp: Date.now() };

    metricsHistory.push(latestMetrics);
    if (metricsHistory.length > MAX_HISTORY)
        metricsHistory.shift();

    // Broadcast to all connected WebSocket clients
    const msg = JSON.stringify(latestMetrics);
    for (const client of wss.clients) {
        if (client.readyState === 1 /* OPEN */)
            client.send(msg);
    }

    return res.status(204).end();
});

// ─── REST: serve latest snapshot ──────────────────────────────────────────────
app.get('/api/metrics', (req, res) => {
    if (!latestMetrics)
        return res.status(503).json({ error: 'No data from agent yet' });
    return res.json(latestMetrics);
});

// ─── REST: serve recent history ───────────────────────────────────────────────
app.get('/api/metrics/history', (req, res) => {
    return res.json(metricsHistory);
});

// ─── WebSocket: send latest snapshot on connect ───────────────────────────────
wss.on('connection', (ws) => {
    if (latestMetrics)
        ws.send(JSON.stringify(latestMetrics));
});

// ─── Start ────────────────────────────────────────────────────────────────────
server.listen(PORT, () => {
    console.log(`[server] API + WebSocket listening on http://localhost:${PORT}`);
});
