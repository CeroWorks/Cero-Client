const WebSocket = require('ws');
const { verifyMinecraftToken } = require('./auth');
const { db } = require('./db');

function ts() {
  return new Date().toISOString();
}

function getFriendUuidsOf(uuid) {
  try {
    const rows = db.prepare(`
      SELECT CASE WHEN user_a = ? THEN user_b ELSE user_a END AS other
      FROM friendships
      WHERE (user_a = ? OR user_b = ?) AND status = 'accepted'
    `).all(uuid, uuid, uuid);
    return rows.map(r => r.other).filter(Boolean);
  } catch (e) {
    console.error(`[WS ${ts()}] getFriendUuidsOf error: ${e.message}`);
    return [];
  }
}


function setupWebSocket(server) {
  const wss = new WebSocket.Server({ server, path: '/ws' });
  const clients = new Map();

  function isOnline(uuid) { return clients.has(uuid); }

  function notify(uuid, payload) {
    const set = clients.get(uuid);
    if (!set) return false;
    const msg = JSON.stringify(payload);
    for (const ws of set) {
      if (ws.readyState === WebSocket.OPEN) ws.send(msg);
    }
    return true;
  }

  function broadcastStatus(uuid, username, status) {
    const friends = getFriendUuidsOf(uuid);
    if (!friends.length) return;
    const payload = {
      type: 'friend_status',
      uuid,
      name: username,
      status,
      lastSeen: Date.now()
    };
    let delivered = 0;
    for (const fu of friends) {
      if (notify(fu, payload)) delivered++;
    }
    if (delivered) {
      console.log(`[WS ${ts()}] PUSH status ${username} -> ${status} to ${delivered}/${friends.length} amis online`);
    }
  }

  wss.on('connection', async (ws, req) => {
    const ip = req.headers['x-forwarded-for']?.split(',')[0].trim()
            || req.socket.remoteAddress;

    const url = new URL(req.url, 'http://localhost');
    const token = url.searchParams.get('token');
    const profile = await verifyMinecraftToken(token);

    if (!profile) {
      console.warn(`[WS ${ts()}] REJECTED ${ip} - unauthorized (bad/missing token)`);
      ws.send(JSON.stringify({ type: 'error', error: 'unauthorized' }));
      return ws.close(4001, 'unauthorized');
    }

    const uuid = profile.uuid;
    const wasOffline = !clients.has(uuid);
    if (!clients.has(uuid)) clients.set(uuid, new Set());
    clients.get(uuid).add(ws);

    const connCount = clients.get(uuid).size;
    console.log(`[WS ${ts()}] CONNECT ${profile.username} (${uuid}) from ${ip} - sessions: ${connCount} - total online: ${clients.size}`);

    db.prepare('UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?')
      .run('online', Date.now(), uuid);

    ws.send(JSON.stringify({ type: 'hello', user: profile }));

    if (wasOffline) {
      broadcastStatus(uuid, profile.username, 'online');
    }

    ws.isAlive = true;
    ws.on('pong', () => { ws.isAlive = true; });

    ws.on('message', (raw) => {
      try {
        const data = JSON.parse(raw);
        if (data.type === 'status' && ['online','ingame','away'].includes(data.value)) {
          db.prepare('UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?')
            .run(data.value, Date.now(), uuid);
          console.log(`[WS ${ts()}] STATUS ${profile.username} (${uuid}) -> ${data.value}`);
          broadcastStatus(uuid, profile.username, data.value);
        }
      } catch (e) {
        console.warn(`[WS ${ts()}] BAD_MESSAGE from ${profile.username} (${uuid}): ${e.message}`);
      }
    });

    ws.on('error', (err) => {
      console.error(`[WS ${ts()}] ERROR ${profile.username} (${uuid}): ${err.message}`);
    });

    ws.on('close', (code, reason) => {
      const set = clients.get(uuid);
      let remaining = 0;
      let wentOffline = false;
      if (set) {
        set.delete(ws);
        remaining = set.size;
        if (set.size === 0) {
          clients.delete(uuid);
          wentOffline = true;
          db.prepare('UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?')
            .run('offline', Date.now(), uuid);
        }
      }
      if (wentOffline) {
        broadcastStatus(uuid, profile.username, 'offline');
      }
      const reasonStr = reason?.toString() || '';
      console.log(`[WS ${ts()}] DISCONNECT ${profile.username} (${uuid}) - code:${code}${reasonStr ? ' reason:'+reasonStr : ''} - remaining sessions: ${remaining} - total online: ${clients.size}`);
    });
  });

  setInterval(() => {
    wss.clients.forEach(ws => {
      if (!ws.isAlive) {
        console.warn(`[WS ${ts()}] TIMEOUT - terminating dead connection`);
        return ws.terminate();
      }
      ws.isAlive = false;
      ws.ping();
    });
  }, 30000);

  console.log(`[WS ${ts()}] WebSocket server ready on /ws`);

  return {
    notify,
    isOnline,
    broadcastStatus,
    getFriendUuidsOf
  };
}

module.exports = setupWebSocket;
