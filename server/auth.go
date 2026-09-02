package main

import (
	"context"
	"encoding/json"
	"net/http"
	"regexp"
	"strings"
	"sync"
	"time"
)

type Profile struct {
	UUID     string `json:"uuid"`
	Username string `json:"username"`
}

type cacheEntry struct {
	profile Profile
	ts      time.Time
}

const cacheTTL = 60 * time.Second

var (
	tokenCache   = map[string]cacheEntry{}
	tokenCacheMu sync.Mutex
	uuidDashRe   = regexp.MustCompile(`^(.{8})(.{4})(.{4})(.{4})(.{12})$`)

	httpClient = &http.Client{Timeout: 5 * time.Second}
)

type mcProfileResponse struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func verifyMinecraftToken(ctx context.Context, accessToken string) *Profile {
	if len(accessToken) < 20 {
		return nil
	}

	tokenCacheMu.Lock()
	if entry, ok := tokenCache[accessToken]; ok && time.Since(entry.ts) < cacheTTL {
		tokenCacheMu.Unlock()
		p := entry.profile
		return &p
	}
	tokenCacheMu.Unlock()

	req, err := http.NewRequestWithContext(ctx, http.MethodGet,
		"https://api.minecraftservices.com/minecraft/profile", nil)
	if err != nil {
		return nil
	}
	req.Header.Set("Authorization", "Bearer "+accessToken)

	resp, err := httpClient.Do(req)
	if err != nil {
		return nil
	}
	defer resp.Body.Close()

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return nil
	}

	var data mcProfileResponse
	if err := json.NewDecoder(resp.Body).Decode(&data); err != nil {
		return nil
	}
	if data.ID == "" || data.Name == "" {
		return nil
	}

	uuid := data.ID
	if uuidDashRe.MatchString(uuid) {
		uuid = uuidDashRe.ReplaceAllString(uuid, "$1-$2-$3-$4-$5")
	}

	profile := Profile{UUID: uuid, Username: data.Name}

	tokenCacheMu.Lock()
	tokenCache[accessToken] = cacheEntry{profile: profile, ts: time.Now()}
	tokenCacheMu.Unlock()

	now := nowMillis()
	_, _ = db.Exec(`
		INSERT INTO users (uuid, username, last_seen, status, created_at)
		VALUES (?, ?, ?, 'online', ?)
		ON CONFLICT(uuid) DO UPDATE SET
			username = excluded.username,
			last_seen = excluded.last_seen,
			status = 'online'
	`, profile.UUID, profile.Username, now, now)

	return &profile
}

type ctxKey string

const userCtxKey ctxKey = "user"

func authMiddleware(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		authHeader := r.Header.Get("Authorization")
		if authHeader == "" || !strings.HasPrefix(authHeader, "Bearer ") {
			writeJSON(w, http.StatusUnauthorized, M{"error": "missing_token"})
			return
		}
		token := strings.TrimSpace(strings.TrimPrefix(authHeader, "Bearer "))
		profile := verifyMinecraftToken(r.Context(), token)
		if profile == nil {
			writeJSON(w, http.StatusUnauthorized, M{"error": "invalid_token"})
			return
		}
		ctx := context.WithValue(r.Context(), userCtxKey, profile)
		next(w, r.WithContext(ctx))
	}
}

func userFromCtx(r *http.Request) *Profile {
	p, _ := r.Context().Value(userCtxKey).(*Profile)
	return p
}

func nowMillis() int64 {
	return time.Now().UnixMilli()
}
