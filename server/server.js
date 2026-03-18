'use strict';

const express = require('express');

const app = express();
app.use(express.json());

// ── In-memory task store ──────────────────────────────────────────────────────
let nextId = 1;
const tasks = [];

const PRIORITIES = ['LOW', 'MEDIUM', 'HIGH'];
const STATUSES   = ['PENDING', 'IN_PROGRESS', 'DONE'];

function todayString() {
  const d = new Date();
  const mm = String(d.getMonth() + 1).padStart(2, '0');
  const dd = String(d.getDate()).padStart(2, '0');
  const yyyy = d.getFullYear();
  return `${mm}/${dd}/${yyyy}`;
}

// ── Routes ────────────────────────────────────────────────────────────────────

// GET /tasks  — list all tasks
app.get('/tasks', (req, res) => {
  res.json(tasks);
});

// POST /tasks  — create a new task
app.post('/tasks', (req, res) => {
  const { name, priority = 'LOW' } = req.body || {};

  if (!name || typeof name !== 'string' || name.trim() === '') {
    return res.status(400).json({ error: 'name is required' });
  }
  if (!PRIORITIES.includes(priority)) {
    return res.status(400).json({ error: `priority must be one of: ${PRIORITIES.join(', ')}` });
  }

  const task = {
    id:       nextId++,
    name:     name.trim(),
    priority,
    status:   'PENDING',
    created:  todayString(),
  };
  tasks.push(task);
  res.status(201).json(task);
});

// GET /tasks/:id  — get a single task
app.get('/tasks/:id', (req, res) => {
  const task = tasks.find(t => t.id === Number(req.params.id));
  if (!task) return res.status(404).json({ error: 'task not found' });
  res.json(task);
});

// PATCH /tasks/:id  — update name, priority, or status
app.patch('/tasks/:id', (req, res) => {
  const task = tasks.find(t => t.id === Number(req.params.id));
  if (!task) return res.status(404).json({ error: 'task not found' });

  const { name, priority, status } = req.body || {};

  if (name !== undefined) {
    if (typeof name !== 'string' || name.trim() === '') {
      return res.status(400).json({ error: 'name must be a non-empty string' });
    }
    task.name = name.trim();
  }
  if (priority !== undefined) {
    if (!PRIORITIES.includes(priority)) {
      return res.status(400).json({ error: `priority must be one of: ${PRIORITIES.join(', ')}` });
    }
    task.priority = priority;
  }
  if (status !== undefined) {
    if (!STATUSES.includes(status)) {
      return res.status(400).json({ error: `status must be one of: ${STATUSES.join(', ')}` });
    }
    task.status = status;
  }

  res.json(task);
});

// DELETE /tasks/:id  — remove a task
app.delete('/tasks/:id', (req, res) => {
  const idx = tasks.findIndex(t => t.id === Number(req.params.id));
  if (idx === -1) return res.status(404).json({ error: 'task not found' });
  tasks.splice(idx, 1);
  res.status(204).send();
});

// GET /stats  — summary counts
app.get('/stats', (req, res) => {
  const count = status => tasks.filter(t => t.status === status).length;
  res.json({
    total:      tasks.length,
    pending:    count('PENDING'),
    inProgress: count('IN_PROGRESS'),
    done:       count('DONE'),
  });
});

// ── Start ─────────────────────────────────────────────────────────────────────
const rawPort = process.env.PORT;
const PORT = rawPort ? parseInt(rawPort, 10) : 3000;
if (!Number.isInteger(PORT) || PORT < 1 || PORT > 65535) {
  console.error(`Invalid PORT value: "${rawPort}". Must be an integer between 1 and 65535.`);
  process.exit(1);
}
app.listen(PORT, () => {
  console.log(`Task Manager server listening on http://localhost:${PORT}`);
});
