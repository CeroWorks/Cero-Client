package main

import (
	"database/sql"
	"log"
	"os"
	"path/filepath"

	_ "github.com/mattn/go-sqlite3"
)

var db *sql.DB

func initDB() *sql.DB {
	dataDir := "./data"
	if v := os.Getenv("DATA_DIR"); v != "" {
		dataDir = v
	}
	if err := os.MkdirAll(dataDir, 0o755); err != nil {
		log.Fatalf("mkdir data dir: %v", err)
	}

	dsn := filepath.Join(dataDir, "cero.db") + "?_journal_mode=WAL&_foreign_keys=on"
	conn, err := sql.Open("sqlite3", dsn)
	if err != nil {
		log.Fatalf("open db: %v", err)
	}

	conn.SetMaxOpenConns(1)

	schema := `
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
		status      TEXT NOT NULL DEFAULT 'pending',
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
		edited_at   INTEGER,
		FOREIGN KEY(from_uuid) REFERENCES users(uuid) ON DELETE CASCADE,
		FOREIGN KEY(to_uuid)   REFERENCES users(uuid) ON DELETE CASCADE
	);

	CREATE INDEX IF NOT EXISTS idx_msg_conv ON messages(from_uuid, to_uuid, sent_at);
	CREATE INDEX IF NOT EXISTS idx_friend_a ON friendships(user_a);
	CREATE INDEX IF NOT EXISTS idx_friend_b ON friendships(user_b);
	`
	if _, err := conn.Exec(schema); err != nil {
		log.Fatalf("migrate schema: %v", err)
	}

	db = conn
	return conn
}

func pair(a, b string) (string, string) {
	if a < b {
		return a, b
	}
	return b, a
}
