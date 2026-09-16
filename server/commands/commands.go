package commands

import (
    "database/sql"
    "fmt"
    "io"
    "sort"
    "strings"
    "time"
)

type HubAPI interface {
    IsOnline(uuid string) bool
    TotalOnline() int
    Snapshot() map[string]int
    Kick(uuid string) int
}

type Deps struct {
    DB       *sql.DB
    Hub      HubAPI
    Writer   io.Writer
    Shutdown func()
}

var (
    deps      Deps
    startedAt = time.Now()
)

func Init(d Deps) {
    if d.Writer == nil {
        d.Writer = io.Discard
    }
    deps = d
}

func out(format string, args ...any) {
    fmt.Fprintf(deps.Writer, format, args...)
}

type Command struct {
    Name string
    Desc string
    Run  func(args []string)
}

var registry = map[string]Command{}

func Register(cmd Command) {
    if cmd.Name == "" || cmd.Run == nil {
        return
    }
    registry[cmd.Name] = cmd
}

func Execute(line string) {
    fields := strings.Fields(line)
    if len(fields) == 0 {
        return
    }
    cmd, ok := registry[fields[0]]
    if !ok {
        out("unknown command %q — type 'help'\n", fields[0])
        return
    }
    cmd.Run(fields[1:])
}

func init() {
    Register(Command{Name: "help", Desc: "list available commands", Run: helpCmd})
    Register(Command{Name: "stop", Desc: "shut down the server", Run: stopCmd})
}

func helpCmd([]string) {
    names := make([]string, 0, len(registry))
    for n := range registry {
        names = append(names, n)
    }
    sort.Strings(names)
    for _, n := range names {
        out("  %-12s %s\n", n, registry[n].Desc)
    }
}

func stopCmd([]string) {
    out("shutting down…\n")
    if deps.Shutdown != nil {
        deps.Shutdown()
    }
}