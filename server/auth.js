const fetch = require('node-fetch');
const { db } = require('./db');

const tokenCache = new Map();
const CACHE_TTL = 60 * 1000;

async function verifyMinecraftToken(accessToken) {
  if (!accessToken || typeof accessToken !== 'string' || accessToken.length < 20) {
    return null;
  }

  const cached = tokenCache.get(accessToken);
  if (cached && Date.now() - cached.ts < CACHE_TTL) {
    return cached.profile;
  }

  try {
    const r = await fetch('https://api.minecraftservices.com/minecraft/profile', {
      headers: { 'Authorization': `Bearer ${accessToken}` },
      timeout: 5000
    });
    if (!r.ok) return null;
    const data = await r.json();
    if (!data.id || !data.name) return null;

    const uuid = data.id.replace(
      /^(.{8})(.{4})(.{4})(.{4})(.{12})$/,
      '$1-$2-$3-$4-$5'
    );

    const profile = { uuid, username: data.name };
    tokenCache.set(accessToken, { profile, ts: Date.now() });

    db.prepare(`
      INSERT INTO users (uuid, username, last_seen, status, created_at)
      VALUES (?, ?, ?, 'online', ?)
      ON CONFLICT(uuid) DO UPDATE SET
        username = excluded.username,
        last_seen = excluded.last_seen,
        status = 'online'
    `).run(uuid, profile.username, Date.now(), Date.now());

    return profile;
  } catch (e) {
    console.error('verifyMinecraftToken:', e.message);
    return null;
  }
}

async function authMiddleware(req, res, next) {
  const auth = req.headers['authorization'];
  if (!auth || !auth.startsWith('Bearer ')) {
    return res.status(401).json({ error: 'missing_token' });
  }
  const token = auth.slice(7).trim();
  const profile = await verifyMinecraftToken(token);
  if (!profile) return res.status(401).json({ error: 'invalid_token' });
  req.user = profile;
  next();
}

module.exports = { verifyMinecraftToken, authMiddleware };
