const express = require('express');
const { db, pair } = require('../db');
const { authMiddleware } = require('../auth');
const router = express.Router();

router.use(authMiddleware);

function areFriends(a, b) {
  const [x, y] = pair(a, b);
  const f = db.prepare('SELECT status FROM friendships WHERE user_a = ? AND user_b = ?').get(x, y);
  return f && f.status === 'accepted';
}

router.get('/:uuid', (req, res) => {
  const me = req.user.uuid;
  const other = req.params.uuid;
  if (!areFriends(me, other)) return res.status(403).json({ error: 'not_friends' });

  const limit  = Math.min(parseInt(req.query.limit) || 50, 100);
  const before = parseInt(req.query.before) || Date.now() + 1;

  const messages = db.prepare(`
    SELECT id, from_uuid, to_uuid, content, sent_at, read_at, edited_at
    FROM messages
    WHERE ((from_uuid = ? AND to_uuid = ?) OR (from_uuid = ? AND to_uuid = ?))
      AND sent_at < ?
    ORDER BY sent_at DESC
    LIMIT ?
  `).all(me, other, other, me, before, limit);

  db.prepare(`
    UPDATE messages SET read_at = ?
    WHERE to_uuid = ? AND from_uuid = ? AND read_at IS NULL
  `).run(Date.now(), me, other);

  res.json({ messages: messages.reverse() });
});

router.post('/', (req, res) => {
  const me = req.user.uuid;
  const { to, content } = req.body || {};

  if (!to || !content) return res.status(400).json({ error: 'missing_fields' });
  if (typeof content !== 'string') return res.status(400).json({ error: 'invalid_content' });

  const clean = content.trim();
  if (clean.length === 0 || clean.length > 1000) {
    return res.status(400).json({ error: 'content_length', max: 1000 });
  }
  if (!areFriends(me, to)) return res.status(403).json({ error: 'not_friends' });

  const now = Date.now();
  const r = db.prepare(`
    INSERT INTO messages (from_uuid, to_uuid, content, sent_at)
    VALUES (?, ?, ?, ?)
  `).run(me, to, clean, now);

  const msg = {
    id: r.lastInsertRowid,
    from_uuid: me,
    to_uuid: to,
    content: clean,
    sent_at: now,
    read_at: null,
    edited_at: null
  };

  req.app.get('ws')?.notify(to, { type: 'message', message: msg, from: req.user });

  res.json({ ok: true, message: msg });
});

router.patch('/:id', (req, res) => {
  const me = req.user.uuid;
  const id = parseInt(req.params.id);
  const { content } = req.body || {};

  if (!id) return res.status(400).json({ error: 'invalid_id' });
  if (typeof content !== 'string') return res.status(400).json({ error: 'invalid_content' });

  const clean = content.trim();
  if (clean.length === 0 || clean.length > 1000) {
    return res.status(400).json({ error: 'content_length', max: 1000 });
  }

  const msg = db.prepare('SELECT * FROM messages WHERE id = ?').get(id);
  if (!msg) return res.status(404).json({ error: 'not_found' });

  if (msg.from_uuid !== me) return res.status(403).json({ error: 'forbidden' });

  const now = Date.now();
  db.prepare('UPDATE messages SET content = ?, edited_at = ? WHERE id = ?').run(clean, now, id);

  const updated = { ...msg, content: clean, edited_at: now };

  req.app.get('ws')?.notify(msg.to_uuid, { type: 'message_edited', message: updated });

  res.json({ ok: true, message: updated });
});

router.delete('/:id', (req, res) => {
  const me = req.user.uuid;
  const id = parseInt(req.params.id);
  if (!id) return res.status(400).json({ error: 'invalid_id' });

  const msg = db.prepare('SELECT * FROM messages WHERE id = ?').get(id);
  if (!msg) return res.status(404).json({ error: 'not_found' });

  if (msg.from_uuid !== me) return res.status(403).json({ error: 'forbidden' });

  db.prepare('DELETE FROM messages WHERE id = ?').run(id);

  req.app.get('ws')?.notify(msg.to_uuid, { type: 'message_deleted', id });

  res.json({ ok: true, id });
});

router.get('/unread/count', (req, res) => {
  const me = req.user.uuid;
  const rows = db.prepare(`
    SELECT from_uuid, COUNT(*) as count
    FROM messages
    WHERE to_uuid = ? AND read_at IS NULL
    GROUP BY from_uuid
  `).all(me);
  res.json({ unread: rows });
});

module.exports = router;