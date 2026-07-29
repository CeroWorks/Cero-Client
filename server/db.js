const Database = require('better-sqlite3');
const path = require('path');
const fs = require('fs');

const dataDir = path.join(__dirname, 'data');
if (!fs.existsSync(dataDir)) fs.mkdirSync(dataDir);

const db = new Database(path.join(dataDir, 'cero.db'));
db.pragma('journal_mode = WAL');
db.pragma('foreign_keys = ON');

db.exec(`
  CREATE TABLE IF NOT EXISTS users (
    uuid        TEXT PRIMARY KEY,
    username    TEXT NOT NULL,
    last_seen   INTEGER NOT NULL DEFAULT 0,
    status      TEXT NOT NULL DEFAULT 'offline',
    created_at  INTEGER NOT NULL
  );

  CREATE TABLE IF NOT EXISTS friendships (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    user_a      TEXT NOT NULL,
    user_b      TEXT NOT NULL,
    status      TEXT NOT NULL DEFAULT 'pending', -- pending | accepted | blocked
    requested_by TEXT NOT NULL,
    created_at  INTEGER NOT NULL,
    UNIQUE(user_a, user_b),
    FOREIGN KEY(user_a) REFERENCES users(uuid) ON DELETE CASCADE,
    FOREIGN KEY(user_b) REFERENCES users(uuid) ON DELETE CASCADE
  );

  CREATE TABLE IF NOT EXISTS messages (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    from_uuid   TEXT NOT NULL,
    to_uuid     TEXT NOT NULL,
    content     TEXT NOT NULL,
    sent_at     INTEGER NOT NULL,
    read_at     INTEGER,
    FOREIGN KEY(from_uuid) REFERENCES users(uuid) ON DELETE CASCADE,
    FOREIGN KEY(to_uuid)   REFERENCES users(uuid) ON DELETE CASCADE
  );

  CREATE INDEX IF NOT EXISTS idx_msg_conv ON messages(from_uuid, to_uuid, sent_at);
  CREATE INDEX IF NOT EXISTS idx_friend_a ON friendships(user_a);
  CREATE INDEX IF NOT EXISTS idx_friend_b ON friendships(user_b);
`);

function pair(a, b) {
  return a < b ? [a, b] : [b, a];
}

module.exports = { db, pair };
