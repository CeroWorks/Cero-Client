package main

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"log"
	"os"
	"strings"
	"sync"

	"ceroclient-server/commands"
)

func init() {
	tty.ansi = enableANSI()
	tty.prompt = "Cero Server > "
	log.SetOutput(tty)
}

type ttyWriter struct {
	mu     sync.Mutex
	out    io.Writer
	prompt string
	live   bool
	ansi   bool
}

var tty = &ttyWriter{
	out: os.Stdout,
}

func (t *ttyWriter) Write(p []byte) (int, error) {
	t.mu.Lock()
	defer t.mu.Unlock()

	if !t.live {
		_, err := t.out.Write(p)
		return len(p), err
	}

	if t.ansi {
		fmt.Fprintf(t.out, "\r\x1b[2K%s%s", p, t.prompt)
	} else {
		fmt.Fprintf(t.out, "\n%s%s", p, t.prompt)
	}

	return len(p), nil
}

func (t *ttyWriter) Printf(format string, args ...any) {
	fmt.Fprintf(t, format, args...)
}

func (t *ttyWriter) showPrompt() {
	t.mu.Lock()
	defer t.mu.Unlock()

	t.live = true
	fmt.Fprint(t.out, t.prompt)
}

func (t *ttyWriter) endPrompt() {
	t.mu.Lock()
	defer t.mu.Unlock()

	t.live = false
}

type Shell struct {
	scanner *bufio.Scanner
}

func newShell() *Shell {
	return &Shell{
		scanner: bufio.NewScanner(os.Stdin),
	}
}

func runShell(ctx context.Context, shutdown context.CancelFunc) {
	commands.Init(commands.Deps{
		DB:     db,
		Hub:    hub,
		Writer: tty,

		Shutdown: func() {
			shutdown()
		},
	})

	commands.InitLua()

	shell := newShell()
	shell.run(ctx)
}

func (s *Shell) run(ctx context.Context) {
	log.Println("shell ready — type 'help'")

	for {
		select {
		case <-ctx.Done():
			tty.endPrompt()
			log.Println("shell stopping...")
			return
		default:
		}

		tty.showPrompt()

		if !s.scanner.Scan() {
			tty.endPrompt()
			break
		}

		line := strings.TrimSpace(s.scanner.Text())
		tty.endPrompt()

		if line == "" {
			continue
		}

		s.execute(line)
	}

	if err := s.scanner.Err(); err != nil {
		log.Printf("shell error: %v", err)
	}

	log.Println("shell closed")
}

func (s *Shell) execute(line string) {
	commands.Execute(line)
}