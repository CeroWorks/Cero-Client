package main

import (
    "bufio"
    "fmt"
    "io"
    "log"
    "os"
    "sort"
    "strings"
    "sync"
)

type Command struct {
    Name string
    Desc string
    Run  func(args []string)
}

var shellCommands = map[string]Command{}

func init() {
    registerShellCommand(Command{
        Name: "help",
        Desc: "list available commands",
        Run:  helpCommand,
    })
    tty.ansi = enableANSI()
    tty.prompt = "Cero Server > "
    log.SetOutput(tty)
}

func registerShellCommand(cmd Command) {
    shellCommands[cmd.Name] = cmd
}

type ttyWriter struct {
    mu     sync.Mutex
    out    io.Writer
    prompt string
    live   bool
    ansi   bool
}

var tty = &ttyWriter{out: os.Stdout}

func (t *ttyWriter) Write(p []byte) (int, error) {
    t.mu.Lock()
    defer t.mu.Unlock()
    if !t.live {
        t.out.Write(p)
        return len(p), nil
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
    stop    chan<- os.Signal
}

func newShell(stop chan<- os.Signal) *Shell {
    return &Shell{
        scanner: bufio.NewScanner(os.Stdin),
        stop:    stop,
    }
}

func runShell(stop chan<- os.Signal) {
    newShell(stop).run()
}

func (s *Shell) run() {
    log.Println("shell ready — type 'help'")
    for {
        tty.showPrompt()
        if !s.scanner.Scan() {
            break
        }
        line := strings.TrimSpace(s.scanner.Text())
        tty.endPrompt()
        s.execute(line)
    }
    if err := s.scanner.Err(); err != nil {
        log.Printf("shell error: %v", err)
    }
    log.Println("shell closed")
}

func (s *Shell) execute(line string) {
    if line == "" {
        return
    }
    fields := strings.Fields(line)
    cmd, ok := shellCommands[fields[0]]
    if !ok {
        tty.Printf("unknown command %q — type 'help'\n", fields[0])
        return
    }
    cmd.Run(fields[1:])
}

func helpCommand([]string) {
    names := make([]string, 0, len(shellCommands))
    for name := range shellCommands {
        names = append(names, name)
    }
    sort.Strings(names)
    for _, name := range names {
        tty.Printf("  %-15s %s\n", name, shellCommands[name].Desc)
    }
}