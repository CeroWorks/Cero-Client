package main

import (
	"database/sql"
	"net/http"
	"strings"
)

type friendOut struct {
	UUID     string `json:"uuid"`
	Name     string `json:"name"`
	Status   string `json:"status"`
	LastSeen int64  `json:"lastSeen"`
	Since    int64  `json:"since"`
}

func handleFriendsList(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r).UUID

	rows, err := db.Query(`
		SELECT f.user_a, f.user_b, f.created_at,
		       ua.username, ua.status, ua.last_seen,
		       ub.username, ub.status, ub.last_seen
		FROM friendships f
		JOIN users ua ON ua.uuid = f.user_a
		JOIN users ub ON ub.uuid = f.user_b
		WHERE (f.user_a = ? OR f.user_b = ?) AND f.status = 'accepted'
	`, me, me)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}
	defer rows.Close()

	friends := []friendOut{}
	for rows.Next() {
		var userA, userB, usernameA, statusA, usernameB, statusB string
		var createdAt, seenA, seenB int64
		if err := rows.Scan(&userA, &userB, &createdAt, &usernameA, &statusA, &seenA, &usernameB, &statusB, &seenB); err != nil {
			continue
		}
		isA := userA == me
		f := friendOut{Since: createdAt}
		if isA {
			f.UUID, f.Name, f.Status, f.LastSeen = userB, usernameB, statusB, seenB
		} else {
			f.UUID, f.Name, f.Status, f.LastSeen = userA, usernameA, statusA, seenA
		}
		friends = append(friends, f)
	}

	writeJSON(w, http.StatusOK, M{"friends": friends})
}

type reqOut struct {
	UUID      string `json:"uuid"`
	Username  string `json:"username"`
	CreatedAt int64  `json:"created_at"`
}

func handleFriendRequests(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r).UUID

	incoming := []reqOut{}
	rows, err := db.Query(`
		SELECT f.requested_by, u.username, f.created_at
		FROM friendships f
		JOIN users u ON u.uuid = f.requested_by
		WHERE (f.user_a = ? OR f.user_b = ?) AND f.status = 'pending' AND f.requested_by != ?
	`, me, me, me)
	if err == nil {
		defer rows.Close()
		for rows.Next() {
			var o reqOut
			if rows.Scan(&o.UUID, &o.Username, &o.CreatedAt) == nil {
				incoming = append(incoming, o)
			}
		}
	}

	outgoing := []reqOut{}
	rows2, err := db.Query(`
		SELECT
			CASE WHEN f.requested_by = ? THEN f.user_b ELSE f.user_a END,
			CASE WHEN f.requested_by = ? THEN ub.username ELSE ua.username END,
			f.created_at
		FROM friendships f
		JOIN users ua ON ua.uuid = f.user_a
		JOIN users ub ON ub.uuid = f.user_b
		WHERE (f.user_a = ? OR f.user_b = ?) AND f.status = 'pending' AND f.requested_by = ?
	`, me, me, me, me, me)
	if err == nil {
		defer rows2.Close()
		for rows2.Next() {
			var o reqOut
			if rows2.Scan(&o.UUID, &o.Username, &o.CreatedAt) == nil {
				outgoing = append(outgoing, o)
			}
		}
	}

	writeJSON(w, http.StatusOK, M{"incoming": incoming, "outgoing": outgoing})
}

type friendship struct {
	ID          int64
	UserA       string
	UserB       string
	Status      string
	RequestedBy string
	CreatedAt   int64
}

func getFriendship(a, b string) (*friendship, error) {
	x, y := pair(a, b)
	var f friendship
	err := db.QueryRow(`SELECT id, user_a, user_b, status, requested_by, created_at FROM friendships WHERE user_a = ? AND user_b = ?`, x, y).
		Scan(&f.ID, &f.UserA, &f.UserB, &f.Status, &f.RequestedBy, &f.CreatedAt)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}
	return &f, nil
}

func getUserBasic(uuid string) (username, status string, ok bool) {
	err := db.QueryRow(`SELECT username, status FROM users WHERE uuid = ?`, uuid).Scan(&username, &status)
	return username, status, err == nil
}

func handleFriendAdd(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r)
	var body struct {
		Username string `json:"username"`
	}
	if err := readJSON(r, &body); err != nil || body.Username == "" || len(body.Username) > 16 {
		writeJSON(w, http.StatusBadRequest, M{"error": "invalid_username"})
		return
	}

	var targetUUID, targetName string
	err := db.QueryRow(`SELECT uuid, username FROM users WHERE username = ? COLLATE NOCASE`, body.Username).
		Scan(&targetUUID, &targetName)
	if err == sql.ErrNoRows {
		writeJSON(w, http.StatusNotFound, M{"error": "user_not_found", "hint": "L'utilisateur doit s'être connecté au moins une fois"})
		return
	}
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}
	if targetUUID == me.UUID {
		writeJSON(w, http.StatusBadRequest, M{"error": "cannot_add_self"})
		return
	}

	a, b := pair(me.UUID, targetUUID)
	existing, err := getFriendship(me.UUID, targetUUID)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}

	if existing != nil {
		switch {
		case existing.Status == "accepted":
			writeJSON(w, http.StatusConflict, M{"error": "already_friends"})
			return
		case existing.Status == "blocked":
			writeJSON(w, http.StatusForbidden, M{"error": "blocked"})
			return
		case existing.RequestedBy == me.UUID:
			writeJSON(w, http.StatusConflict, M{"error": "already_requested"})
			return
		}

		if _, err := db.Exec(`UPDATE friendships SET status = 'accepted' WHERE id = ?`, existing.ID); err != nil {
			writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
			return
		}

		hub.notify(targetUUID, M{"type": "friend_accepted", "by": me})
		if hub.isOnline(me.UUID) {
			if uname, status, ok := getUserBasic(me.UUID); ok {
				hub.notify(targetUUID, M{"type": "friend_status", "uuid": me.UUID, "name": uname, "status": status, "lastSeen": nowMillis()})
			}
		}
		if hub.isOnline(targetUUID) {
			if uname, status, ok := getUserBasic(targetUUID); ok {
				hub.notify(me.UUID, M{"type": "friend_status", "uuid": targetUUID, "name": uname, "status": status, "lastSeen": nowMillis()})
			}
		}

		writeJSON(w, http.StatusOK, M{"ok": true, "status": "accepted"})
		return
	}

	if _, err := db.Exec(`
		INSERT INTO friendships (user_a, user_b, status, requested_by, created_at)
		VALUES (?, ?, 'pending', ?, ?)
	`, a, b, me.UUID, nowMillis()); err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}

	hub.notify(targetUUID, M{"type": "friend_request", "from": me})
	writeJSON(w, http.StatusOK, M{"ok": true, "status": "pending"})
}

func handleFriendAccept(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r)
	var body struct {
		UUID string `json:"uuid"`
	}
	if err := readJSON(r, &body); err != nil || body.UUID == "" {
		writeJSON(w, http.StatusBadRequest, M{"error": "missing_uuid"})
		return
	}

	f, err := getFriendship(me.UUID, body.UUID)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}
	if f == nil || f.Status != "pending" {
		writeJSON(w, http.StatusNotFound, M{"error": "no_request"})
		return
	}
	if f.RequestedBy == me.UUID {
		writeJSON(w, http.StatusBadRequest, M{"error": "cannot_accept_own"})
		return
	}

	if _, err := db.Exec(`UPDATE friendships SET status = 'accepted' WHERE id = ?`, f.ID); err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}

	hub.notify(body.UUID, M{"type": "friend_accepted", "by": me})
	if hub.isOnline(me.UUID) {
		if uname, status, ok := getUserBasic(me.UUID); ok {
			hub.notify(body.UUID, M{"type": "friend_status", "uuid": me.UUID, "name": uname, "status": status, "lastSeen": nowMillis()})
		}
	}
	if hub.isOnline(body.UUID) {
		if uname, status, ok := getUserBasic(body.UUID); ok {
			hub.notify(me.UUID, M{"type": "friend_status", "uuid": body.UUID, "name": uname, "status": status, "lastSeen": nowMillis()})
		}
	}

	writeJSON(w, http.StatusOK, M{"ok": true})
}

func handleFriendDelete(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r)
	other := strings.TrimPrefix(r.URL.Path, "/api/friends/")
	if other == "" {
		writeJSON(w, http.StatusNotFound, M{"error": "not_found"})
		return
	}

	existing, err := getFriendship(me.UUID, other)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}
	if existing == nil {
		writeJSON(w, http.StatusNotFound, M{"error": "not_found"})
		return
	}

	a, b := pair(me.UUID, other)
	if _, err := db.Exec(`DELETE FROM friendships WHERE user_a = ? AND user_b = ?`, a, b); err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}

	if existing.Status == "pending" {
		hub.notify(other, M{"type": "friend_request_declined", "by": me.UUID})
	} else {
		hub.notify(other, M{"type": "friend_removed", "by": me.UUID})
	}

	writeJSON(w, http.StatusOK, M{"ok": true})
}

func handleFriendBlock(w http.ResponseWriter, r *http.Request) {
	me := userFromCtx(r)
	var body struct {
		UUID string `json:"uuid"`
	}
	if err := readJSON(r, &body); err != nil || body.UUID == "" || body.UUID == me.UUID {
		writeJSON(w, http.StatusBadRequest, M{"error": "invalid"})
		return
	}

	a, b := pair(me.UUID, body.UUID)
	_, err := db.Exec(`
		INSERT INTO friendships (user_a, user_b, status, requested_by, created_at)
		VALUES (?, ?, 'blocked', ?, ?)
		ON CONFLICT(user_a, user_b) DO UPDATE SET status = 'blocked', requested_by = excluded.requested_by
	`, a, b, me.UUID, nowMillis())
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
		return
	}

	writeJSON(w, http.StatusOK, M{"ok": true})
}
