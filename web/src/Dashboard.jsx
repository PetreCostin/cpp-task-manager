import { useState, useEffect, useRef } from 'react';

const WS_URL = `ws://${window.location.hostname}:3001`;
const POLL_MS = 3000;

function GaugeBar({ value, color }) {
  const clamped = Math.min(100, Math.max(0, value));
  return (
    <div style={{ background: '#2a2a3a', borderRadius: 6, overflow: 'hidden', height: 18 }}>
      <div style={{
        width: `${clamped}%`,
        height: '100%',
        background: color,
        transition: 'width 0.4s ease',
        borderRadius: 6,
      }} />
    </div>
  );
}

function MetricCard({ title, value, subtitle, color }) {
  return (
    <div style={{
      background: '#1e1e2e',
      border: `1px solid ${color}44`,
      borderRadius: 10,
      padding: '16px 20px',
      minWidth: 200,
      flex: 1,
    }}>
      <div style={{ color: '#888', fontSize: 12, marginBottom: 4 }}>{title}</div>
      <div style={{ color, fontSize: 32, fontWeight: 700, lineHeight: 1.1 }}>{value}</div>
      {subtitle && <div style={{ color: '#666', fontSize: 12, marginTop: 6 }}>{subtitle}</div>}
    </div>
  );
}

export default function Dashboard() {
  const [metrics, setMetrics] = useState(null);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState(null);
  const wsRef = useRef(null);

  useEffect(() => {
    let pollTimer;

    function connectWs() {
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen  = () => { setConnected(true); setError(null); };
      ws.onclose = () => {
        setConnected(false);
        // Fall back to HTTP polling if WebSocket is unavailable
        pollTimer = setInterval(fetchHttp, POLL_MS);
      };
      ws.onerror = () => setError('WebSocket error – falling back to HTTP polling');
      ws.onmessage = (evt) => {
        try { setMetrics(JSON.parse(evt.data)); } catch {}
      };
    }

    async function fetchHttp() {
      try {
        const r = await fetch('/api/metrics');
        if (r.ok) setMetrics(await r.json());
      } catch {
        setError('Cannot reach server');
      }
    }

    connectWs();
    fetchHttp();  // initial load

    return () => {
      wsRef.current?.close();
      clearInterval(pollTimer);
    };
  }, []);

  const fmtUptime = (secs) => {
    if (!secs) return '—';
    const d = Math.floor(secs / 86400);
    const h = Math.floor((secs % 86400) / 3600);
    const m = Math.floor((secs % 3600) / 60);
    return `${d}d ${h}h ${m}m`;
  };

  const fmtMem = (kb) => {
    if (!kb) return '—';
    if (kb > 1024 * 1024) return `${(kb / 1024 / 1024).toFixed(1)} GB`;
    return `${(kb / 1024).toFixed(0)} MB`;
  };

  return (
    <div style={{ padding: '28px 32px', fontFamily: 'monospace', color: '#eee', minHeight: '100vh', background: '#12121c' }}>
      {/* Header */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 28 }}>
        <h1 style={{ margin: 0, fontSize: 22, color: '#7dd3fc' }}>
          ⚙ CPP Distributed Dashboard
        </h1>
        <div style={{
          fontSize: 12, padding: '4px 12px', borderRadius: 20,
          background: connected ? '#166534' : '#7f1d1d',
          color: connected ? '#86efac' : '#fca5a5',
        }}>
          {connected ? '● Live' : '○ Polling'}
        </div>
      </div>

      {error && (
        <div style={{ background: '#3b1f1f', border: '1px solid #f87171', borderRadius: 8, padding: '10px 16px', marginBottom: 20, color: '#f87171', fontSize: 13 }}>
          {error}
        </div>
      )}

      {!metrics ? (
        <div style={{ color: '#666', marginTop: 60, textAlign: 'center', fontSize: 16 }}>
          Waiting for agent data…
        </div>
      ) : (
        <>
          {/* Top metric cards */}
          <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap', marginBottom: 28 }}>
            <MetricCard
              title="CPU Usage"
              value={`${metrics.cpuUsagePercent?.toFixed(1)}%`}
              color="#f472b6"
              subtitle="All cores"
            />
            <MetricCard
              title="Memory Used"
              value={`${metrics.memUsedPercent?.toFixed(1)}%`}
              color="#818cf8"
              subtitle={`${fmtMem(metrics.memTotalKb - metrics.memAvailableKb)} / ${fmtMem(metrics.memTotalKb)}`}
            />
            <MetricCard
              title="Uptime"
              value={fmtUptime(metrics.uptimeSeconds)}
              color="#34d399"
              subtitle="System uptime"
            />
          </div>

          {/* Gauge bars */}
          <div style={{ background: '#1e1e2e', borderRadius: 10, padding: '18px 22px', marginBottom: 24 }}>
            <div style={{ marginBottom: 14 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 13, marginBottom: 6 }}>
                <span style={{ color: '#f472b6' }}>CPU</span>
                <span>{metrics.cpuUsagePercent?.toFixed(1)}%</span>
              </div>
              <GaugeBar value={metrics.cpuUsagePercent} color="#f472b6" />
            </div>
            <div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 13, marginBottom: 6 }}>
                <span style={{ color: '#818cf8' }}>Memory</span>
                <span>{metrics.memUsedPercent?.toFixed(1)}%</span>
              </div>
              <GaugeBar value={metrics.memUsedPercent} color="#818cf8" />
            </div>
          </div>

          {/* Top processes */}
          {metrics.topProcesses?.length > 0 && (
            <div style={{ background: '#1e1e2e', borderRadius: 10, padding: '18px 22px' }}>
              <div style={{ color: '#7dd3fc', fontSize: 14, marginBottom: 14, fontWeight: 600 }}>
                Top Processes (by memory)
              </div>
              <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 13 }}>
                <thead>
                  <tr style={{ color: '#666', borderBottom: '1px solid #2a2a3a' }}>
                    <th style={{ textAlign: 'left', padding: '4px 8px' }}>PID</th>
                    <th style={{ textAlign: 'left', padding: '4px 8px' }}>Name</th>
                    <th style={{ textAlign: 'right', padding: '4px 8px' }}>Memory</th>
                  </tr>
                </thead>
                <tbody>
                  {metrics.topProcesses.map((p) => (
                    <tr key={p.pid} style={{ borderBottom: '1px solid #1a1a28' }}>
                      <td style={{ padding: '6px 8px', color: '#888' }}>{p.pid}</td>
                      <td style={{ padding: '6px 8px', color: '#e2e8f0' }}>{p.name}</td>
                      <td style={{ padding: '6px 8px', textAlign: 'right', color: '#818cf8' }}>{fmtMem(p.memKb)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}

          <div style={{ marginTop: 20, color: '#444', fontSize: 11, textAlign: 'right' }}>
            Last update: {metrics.timestamp ? new Date(metrics.timestamp).toLocaleTimeString() : '—'}
          </div>
        </>
      )}
    </div>
  );
}
