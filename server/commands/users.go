package commands

import (
    "fmt"
    "time"
	"database/sql"
)

type userRow struct {
    UUID      string
    Username  string
    Status    string
    LastSeen  int64
    CreatedAt int64
}

func findUser(q string) *userRow {
    var u userRow
    err := deps.DB.QueryRow(`
        SELECT uuid, username, status, last_seen, created_at
        FROM users WHERE username = ? COLLATE NOCASE OR uuid = ?`, q, q).
        Scan(&u.UUID, &u.Username, &u.Status, &u.LastSeen, &u.CreatedAt)
    if err != nil {
        return nil
    }
    return &u
}

func relTime(ms int64) string {
    if ms <= 0 {
        return "never"
    }
    d := time.Since(time.UnixMilli(ms))
    switch {
    case d < time.Minute:
        return fmt.Sprintf("%.0fs ago", d.Seconds())
    case d < time.Hour:
        return fmt.Sprintf("%dm ago", int(d.Minutes()))
    case d < 24*time.Hour:
        return fmt.Sprintf("%dh ago", int(d.Hours()))
    default:
        return fmt.Sprintf("%dd ago", int(d.Hours()/24))
    }
}

func init() {
    Register(Command{Name: "users", Desc: "users list [search] | info | delete", Run: usersCmd})
}

func usersCmd(args []string) {
    if len(args) == 0 {
        usersList(nil)
        return
    }
    sub, rest := args[0], args[1:]
    switch sub {
    case "list":
        usersList(rest)
    case "info":
        if len(rest) == 0 {
            out("usage: users info <name|uuid>\n")
            return
        }
        usersInfo(rest[0])
    case "delete", "del", "rm":
        if len(rest) == 0 {
            out("usage: users delete <name|uuid> confirm\n")
            return
        }
        usersDelete(rest)
    default:
        out("usage: users list [search] | users info <name> | users delete <name> confirm\n")
    }
}

func usersList(args []string) {
    base := `SELECT uuid, username, status, last_seen, created_at FROM users`
    var (
        rows *sql.Rows
        err  error
    )
    if len(args) > 0 {
        rows, err = deps.DB.Query(base+` WHERE username LIKE ? COLLATE NOCASE ORDER BY last_seen DESC`, "%"+args[0]+"%")
    } else {
        rows, err = deps.DB.Query(base + ` ORDER BY last_seen DESC`)
    }
    if err != nil {
        out("db error: %v\n", err)
        return
    }
    defer rows.Close()

    snap := map[string]int{}
    if deps.Hub != nil {
        snap = deps.Hub.Snapshot()
    }

    out("  %-17s %-9s %-9s %-12s\n", "USERNAME", "STATUS", "SESSIONS", "LAST SEEN")
    count := 0
    for rows.Next() {
        var u userRow
        if rows.Scan(&u.UUID, &u.Username, &u.Status, &u.LastSeen, &u.CreatedAt) != nil {
            continue
        }
        out("  %-17s %-9s %-9d %s\n", u.Username, u.Status, snap[u.UUID], relTime(u.LastSeen))
        count++
    }
    out("  %d user(s)\n", count)
}

func usersInfo(q string) {
    u := findUser(q)
    if u == nil {
        out("user %q not found\n", q)
        return
    }
    var friends, sent, received int
    deps.DB.QueryRow(`SELECT COUNT(*) FROM friendships WHERE (user_a=? OR user_b=?) AND status='accepted'`, u.UUID, u.UUID).Scan(&friends)
    deps.DB.QueryRow(`SELECT COUNT(*) FROM messages WHERE from_uuid=?`, u.UUID).Scan(&sent)
    deps.DB.QueryRow(`SELECT COUNT(*) FROM messages WHERE to_uuid=?`, u.UUID).Scan(&received)

    sessions := 0
    ws := "disconnected"
    if deps.Hub != nil {
        sessions = deps.Hub.Snapshot()[u.UUID]
        if sessions > 0 {
            ws = fmt.Sprintf("connected (%d session(s))", sessions)
        }
    }
    out(`  uuid       %s
  username   %s
  status     %s — ws: %s
  created    %s
  last seen  %s
  friends    %d
  messages   %d sent / %d received
`, u.UUID, u.Username, u.Status, ws,
        time.UnixMilli(u.CreatedAt).Format("2006-01-02 15:04"),
        relTime(u.LastSeen), friends, sent, received)
}

func usersDelete(args []string) {
    u := findUser(args[0])
    if u == nil {
        out("user %q not found\n", args[0])
        return
    }
    if len(args) < 2 || args[1] != "confirm" {
        out("  ⚠ this deletes %q AND all their messages + friendships (cascade)\n", u.Username)
        out("  to confirm: users delete %s confirm\n", u.Username)
        return
    }
    if deps.Hub != nil {
        deps.Hub.Kick(u.UUID)
    }
    res, err := deps.DB.Exec(`DELETE FROM users WHERE uuid = ?`, u.UUID)
    if err != nil {
        out("db error: %v\n", err)
        return
    }
    n, _ := res.RowsAffected()
    out("user %q deleted (%d row)\n", u.Username, n)
}