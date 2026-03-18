const express = require('express');
const app = express();

app.use(express.json({ limit: '100kb' }));

let latestMetrics = {};

app.post('/metrics', (req, res) => {
    if (!req.body || typeof req.body !== 'object' || Array.isArray(req.body)) {
        return res.status(400).json({ error: 'Request body must be a JSON object' });
    }
    latestMetrics = req.body;
    console.log("Received:", latestMetrics);
    res.sendStatus(200);
});

app.get('/metrics', (req, res) => {
    res.json(latestMetrics);
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`API running on port ${PORT}`);
});
