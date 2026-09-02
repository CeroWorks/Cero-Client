package main

import (
	"encoding/json"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

const (
	pingInterval = 30 * time.Second
	pongWait     = 60 * time.Second
	writeWait    = 10 * time.Second
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type wsConn struct {
	conn *websocket.Conn
	send chan []byte
	mu   sync.Mutex
}

func (c *wsConn) writeJSON(v any) {
	b, err := json.Marshal(v)
	if err != nil {
		return
	}
	select {
	case c.send <- b:
	default:

	}
}

type Hub struct {
	mu      sync.Mutex
	clients map[string]map[*wsConn]bool
}

func newHub() *Hub {
	return &Hub{clients: make(map[string]map[*wsConn]bool)}
}

func (h *Hub) isOnline(uuid string) bool {
	h.mu.Lock()
	defer h.mu.Unlock()
	set, ok := h.clients[uuid]
	return ok && len(set) > 0
}

func (h *Hub) notify(uuid string, payload any) bool {
	h.mu.Lock()
	set, ok := h.clients[uuid]
	var conns []*wsConn
	if ok {
		for c := range set {
			conns = append(conns, c)
		}
	}
	h.mu.Unlock()
	if len(conns) == 0 {
		return false
	}
	for _, c := range conns {
		c.writeJSON(payload)
	}
	return true
}

func (h *Hub) getFriendUUIDsOf(uuid string) []string {
	rows, err := db.Query(`
		SELECT CASE WHEN user_a = ? THEN user_b ELSE user_a END AS other
		FROM friendships
		WHERE (user_a = ? OR user_b = ?) AND status = 'accepted'
	`, uuid, uuid, uuid)
	if err != nil {
		log.Printf("[WS %s] getFriendUuidsOf error: %v", ts(), err)
		return nil
	}
	defer rows.Close()

	var out []string
	for rows.Next() {
		var other string
		if err := rows.Scan(&other); err == nil && other != "" {
			out = append(out, other)
		}
	}
	return out
}

func (h *Hub) broadcastStatus(uuid, username, status string) {
	friends := h.getFriendUUIDsOf(uuid)
	if len(friends) == 0 {
		return
	}
	payload := M{
		"type":     "friend_status",
		"uuid":     uuid,
		"name":     username,
		"status":   status,
		"lastSeen": nowMillis(),
	}
	delivered := 0
	for _, fu := range friends {
		if h.notify(fu, payload) {
			delivered++
		}
	}
	if delivered > 0 {
		log.Printf("[WS %s] PUSH status %s -> %s to %d/%d amis online",
			ts(), username, status, delivered, len(friends))
	}
}

func (h *Hub) add(uuid string, c *wsConn) (wasOffline bool, count int) {
	h.mu.Lock()
	defer h.mu.Unlock()
	set, ok := h.clients[uuid]
	if !ok {
		set = make(map[*wsConn]bool)
		h.clients[uuid] = set
	}
	wasOffline = len(set) == 0
	set[c] = true
	return wasOffline, len(set)
}

func (h *Hub) remove(uuid string, c *wsConn) (remaining int, wentOffline bool) {
	h.mu.Lock()
	defer h.mu.Unlock()
	set, ok := h.clients[uuid]
	if !ok {
		return 0, false
	}
	delete(set, c)
	remaining = len(set)
	if remaining == 0 {
		delete(h.clients, uuid)
		wentOffline = true
	}
	return remaining, wentOffline
}

func (h *Hub) totalOnline() int {
	h.mu.Lock()
	defer h.mu.Unlock()
	return len(h.clients)
}

func ts() string {
	return time.Now().UTC().Format(time.RFC3339)
}

func (h *Hub) handleWS(w http.ResponseWriter, r *http.Request) {
	ip := r.Header.Get("X-Forwarded-For")
	if ip != "" {
		ip = strings.TrimSpace(strings.Split(ip, ",")[0])
	} else {
		ip = r.RemoteAddr
	}

	token := r.URL.Query().Get("token")
	profile := verifyMinecraftToken(r.Context(), token)

	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}

	if profile == nil {
		log.Printf("[WS %s] REJECTED %s - unauthorized (bad/missing token)", ts(), ip)
		_ = conn.WriteJSON(M{"type": "error", "error": "unauthorized"})
		_ = conn.WriteControl(websocket.CloseMessage,
			websocket.FormatCloseMessage(4001, "unauthorized"), time.Now().Add(writeWait))
		_ = conn.Close()
		return
	}

	uuid := profile.UUID
	c := &wsConn{conn: conn, send: make(chan []byte, 32)}

	wasOffline, sessions := h.add(uuid, c)
	log.Printf("[WS %s] CONNECT %s (%s) from %s - sessions: %d - total online: %d",
		ts(), profile.Username, uuid, ip, sessions, h.totalOnline())

	_, _ = db.Exec(`UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?`,
		"online", nowMillis(), uuid)

	c.writeJSON(M{"type": "hello", "user": profile})

	if wasOffline {
		h.broadcastStatus(uuid, profile.Username, "online")
	}

	done := make(chan struct{})
	go c.writePump(done)
	h.readPump(c, uuid, profile)
	close(done)

	remaining, wentOffline := h.remove(uuid, c)
	if wentOffline {
		_, _ = db.Exec(`UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?`,
			"offline", nowMillis(), uuid)
		h.broadcastStatus(uuid, profile.Username, "offline")
	}
	log.Printf("[WS %s] DISCONNECT %s (%s) - remaining sessions: %d - total online: %d",
		ts(), profile.Username, uuid, remaining, h.totalOnline())
}

func (c *wsConn) writePump(done <-chan struct{}) {
	ticker := time.NewTicker(pingInterval)
	defer ticker.Stop()
	for {
		select {
		case msg, ok := <-c.send:
			if !ok {
				return
			}
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
				return
			}
		case <-ticker.C:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				log.Printf("[WS %s] TIMEOUT - terminating dead connection", ts())
				return
			}
		case <-done:
			return
		}
	}
}

func (h *Hub) readPump(c *wsConn, uuid string, profile *Profile) {
	defer c.conn.Close()
	_ = c.conn.SetReadDeadline(time.Now().Add(pongWait))
	c.conn.SetPongHandler(func(string) error {
		return c.conn.SetReadDeadline(time.Now().Add(pongWait))
	})

	for {
		_, raw, err := c.conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("[WS %s] ERROR %s (%s): %v", ts(), profile.Username, uuid, err)
			}
			return
		}

		var data struct {
			Type  string `json:"type"`
			Value string `json:"value"`
		}
		if err := json.Unmarshal(raw, &data); err != nil {
			log.Printf("[WS %s] BAD_MESSAGE from %s (%s): %v", ts(), profile.Username, uuid, err)
			continue
		}
		if data.Type == "status" && (data.Value == "online" || data.Value == "ingame" || data.Value == "away") {
			_, _ = db.Exec(`UPDATE users SET status = ?, last_seen = ? WHERE uuid = ?`,
				data.Value, nowMillis(), uuid)
			log.Printf("[WS %s] STATUS %s (%s) -> %s", ts(), profile.Username, uuid, data.Value)
			h.broadcastStatus(uuid, profile.Username, data.Value)
		}
	}
}
