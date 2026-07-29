const express = require('express');
const { db, pair } = require('../db');
const { authMiddleware } = require('../auth');
const router = express.Router();

router.use(authMiddleware);

router.get('/', (req, res) => {
  const me = req.user.uuid;
  const rows = db.prepare(`
    SELECT f.*, 
           ua.username AS username_a, ua.status AS status_a, ua.last_seen AS seen_a,
           ub.username AS username_b, ub.status AS status_b, ub.last_seen AS seen_b
    FROM friendships f
    JOIN users ua ON ua.uuid = f.user_a
    JOIN users ub ON ub.uuid = f.user_b
    WHERE (f.user_a = ? OR f.user_b = ?) AND f.status = 'accepted'
  `).all(me, me);

  const friends = rows.map(r => {
    const isA = r.user_a === me;
    return {
      uuid:     isA ? r.user_b     : r.user_a,
      name:     isA ? r.username_b : r.username_a,
      status:   isA ? r.status_b   : r.status_a,
      lastSeen: isA ? r.seen_b     : r.seen_a,
      since:    r.created_at
    };
  });

  res.json({ friends });
});

router.get('/requests', (req, res) => {
  const me = req.user.uuid;
  
  const incoming = db.prepare(`
    SELECT f.requested_by AS uuid, u.username, f.created_at
    FROM friendships f
    JOIN users u ON u.uuid = f.requested_by
    WHERE (f.user_a = ? OR f.user_b = ?)
      AND f.status = 'pending'
      AND f.requested_by != ?
  `).all(me, me, me);

  const outgoing = db.prepare(`
    SELECT 
      CASE WHEN f.requested_by = ? THEN f.user_b ELSE f.user_a END AS uuid,
      CASE WHEN f.requested_by = ? THEN ub.username ELSE ua.username END AS username,
      f.created_at
    FROM friendships f
    JOIN users ua ON ua.uuid = f.user_a
    JOIN users ub ON ub.uuid = f.user_b
    WHERE (f.user_a = ? OR f.user_b = ?)
      AND f.status = 'pending'
      AND f.requested_by = ?
  `).all(me, me, me, me, me);

  res.json({ incoming, outgoing });
});

router.post('/add', (req, res) => {
  const me = req.user.uuid;
  const { username } = req.body || {};
  if (!username || typeof username !== 'string' || username.length > 16) {
    return res.status(400).json({ error: 'invalid_username' });
  }

  const target = db.prepare('SELECT uuid, username FROM users WHERE username = ? COLLATE NOCASE').get(username);
  if (!target) return res.status(404).json({ error: 'user_not_found', hint: 'L\'utilisateur doit s\'être connecté au moins une fois' });
  if (target.uuid === me) return res.status(400).json({ error: 'cannot_add_self' });

  const [a, b] = pair(me, target.uuid);
  const existing = db.prepare('SELECT * FROM friendships WHERE user_a = ? AND user_b = ?').get(a, b);

  const ws = req.app.get('ws');

  if (existing) {
    if (existing.status === 'accepted') return res.status(409).json({ error: 'already_friends' });
    if (existing.status === 'blocked')  return res.status(403).json({ error: 'blocked' });
    if (existing.requested_by === me)   return res.status(409).json({ error: 'already_requested' });

    db.prepare('UPDATE friendships SET status = ? WHERE id = ?').run('accepted', existing.id);

    if (ws) {
      ws.notify(target.uuid, { type: 'friend_accepted', by: req.user });

      if (ws.isOnline(me)) {
        const meRow = db.prepare('SELECT username, status FROM users WHERE uuid = ?').get(me);
        ws.notify(target.uuid, { type: 'friend_status', uuid: me, name: meRow.username, status: meRow.status, lastSeen: Date.now() });
      }
      if (ws.isOnline(target.uuid)) {
        const otherRow = db.prepare('SELECT username, status FROM users WHERE uuid = ?').get(target.uuid);
        ws.notify(me, { type: 'friend_status', uuid: target.uuid, name: otherRow.username, status: otherRow.status, lastSeen: Date.now() });
      }
    }

    return res.json({ ok: true, status: 'accepted' });
  }

  db.prepare(`
    INSERT INTO friendships (user_a, user_b, status, requested_by, created_at)
    VALUES (?, ?, 'pending', ?, ?)
  `).run(a, b, me, Date.now());

  ws?.notify(target.uuid, { type: 'friend_request', from: req.user });

  res.json({ ok: true, status: 'pending' });
});


router.post('/accept', (req, res) => {
  const me = req.user.uuid;
  const { uuid } = req.body || {};
  if (!uuid) return res.status(400).json({ error: 'missing_uuid' });

  const [a, b] = pair(me, uuid);
  const f = db.prepare('SELECT * FROM friendships WHERE user_a = ? AND user_b = ?').get(a, b);
  if (!f || f.status !== 'pending') return res.status(404).json({ error: 'no_request' });
  if (f.requested_by === me) return res.status(400).json({ error: 'cannot_accept_own' });

  db.prepare('UPDATE friendships SET status = ? WHERE id = ?').run('accepted', f.id);

  const ws = req.app.get('ws');
  ws?.notify(uuid, { type: 'friend_accepted', by: req.user });

  if (ws) {
    if (ws.isOnline(me)) {
      const meRow = db.prepare('SELECT username, status FROM users WHERE uuid = ?').get(me);
      ws.notify(uuid, { type: 'friend_status', uuid: me, name: meRow.username, status: meRow.status, lastSeen: Date.now() });
    }
    if (ws.isOnline(uuid)) {
      const otherRow = db.prepare('SELECT username, status FROM users WHERE uuid = ?').get(uuid);
      ws.notify(me, { type: 'friend_status', uuid, name: otherRow.username, status: otherRow.status, lastSeen: Date.now() });
    }
  }

  res.json({ ok: true });
});

router.delete('/:uuid', (req, res) => {
  const me = req.user.uuid;
  const other = req.params.uuid;
  const [a, b] = pair(me, other);
  
  const existing = db.prepare('SELECT * FROM friendships WHERE user_a = ? AND user_b = ?').get(a, b);
  if (!existing) return res.status(404).json({ error: 'not_found' });
  
  db.prepare('DELETE FROM friendships WHERE user_a = ? AND user_b = ?').run(a, b);
  
  const ws = req.app.get('ws');
  
  if (existing.status === 'pending') {
    ws?.notify(other, { type: 'friend_request_declined', by: req.user.uuid });
  } else {
    ws?.notify(other, { type: 'friend_removed', by: req.user.uuid });
  }
  
  res.json({ ok: true });
});

router.post('/block', (req, res) => {
  const me = req.user.uuid;
  const { uuid } = req.body || {};
  if (!uuid || uuid === me) return res.status(400).json({ error: 'invalid' });
  const [a, b] = pair(me, uuid);
  db.prepare(`
    INSERT INTO friendships (user_a, user_b, status, requested_by, created_at)
    VALUES (?, ?, 'blocked', ?, ?)
    ON CONFLICT(user_a, user_b) DO UPDATE SET status = 'blocked', requested_by = excluded.requested_by
  `).run(a, b, me, Date.now());
  res.json({ ok: true });
});

module.exports = router;
