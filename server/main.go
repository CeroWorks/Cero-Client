package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"
)

var hub *Hub

func main() {
	initDB()
	defer db.Close()

	hub = newHub()

	mux := http.NewServeMux()

	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, http.StatusOK, M{"ok": true, "time": time.Now().UnixMilli()})
	})

	mux.HandleFunc("/ws", hub.handleWS)

	mux.HandleFunc("GET /api/friends", authMiddleware(handleFriendsList))
	mux.HandleFunc("GET /api/friends/requests", authMiddleware(handleFriendRequests))
	mux.HandleFunc("POST /api/friends/add", authMiddleware(handleFriendAdd))
	mux.HandleFunc("POST /api/friends/accept", authMiddleware(handleFriendAccept))
	mux.HandleFunc("POST /api/friends/block", authMiddleware(handleFriendBlock))
	mux.HandleFunc("DELETE /api/friends/{uuid}", authMiddleware(handleFriendDelete))

	mux.HandleFunc("GET /api/messages/unread/count", authMiddleware(handleUnreadCount))
	mux.HandleFunc("GET /api/messages/{uuid}", authMiddleware(handleMessagesGet))
	mux.HandleFunc("POST /api/messages", authMiddleware(handleMessagesSend))
	mux.HandleFunc("PATCH /api/messages/{id}", authMiddleware(handleMessagesEdit))
	mux.HandleFunc("DELETE /api/messages/{id}", authMiddleware(handleMessagesDelete))

	var handler http.Handler = mux
	handler = messagesRateLimit(handler)
	handler = generalAPIRateLimit(handler)
	handler = corsMiddleware(handler)
	handler = securityHeaders(handler)
	handler = recoverMiddleware(handler)

	port := os.Getenv("PORT")
	if port == "" {
		port = "3134"
	}

	srv := &http.Server{
		Addr:         ":" + port,
		Handler:      handler,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 15 * time.Second,
		IdleTimeout:  60 * time.Second,
	}

	go func() {
		log.Printf("CeroClient server on http://localhost:%s", port)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("listen: %v", err)
		}
	}()

	gracefulShutdown(srv)
}

func gracefulShutdown(srv *http.Server) {
	stop := make(chan os.Signal, 1)
	signal.Notify(stop, syscall.SIGTERM, syscall.SIGINT)
	<-stop

	log.Println("shutdown signal received, draining connections...")

	ctx, cancel := context.WithTimeout(context.Background(), 25*time.Second)
	defer cancel()

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		if err := srv.Shutdown(ctx); err != nil {
			log.Printf("shutdown error: %v", err)
		}
	}()
	wg.Wait()

	log.Println("shutdown complete")
}

func recoverMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		defer func() {
			if err := recover(); err != nil {
				log.Printf("panic: %v", err)
				writeJSON(w, http.StatusInternalServerError, M{"error": "internal"})
			}
		}()
		next.ServeHTTP(w, r)
	})
}
