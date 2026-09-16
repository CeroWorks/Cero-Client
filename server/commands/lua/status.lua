local cero = require("cero")

cero.define("status", "server status (uptime, users, messages)", function()
    local users    = cero.queryrow("SELECT COUNT(*) AS n FROM users")
    local accepted = cero.queryrow("SELECT COUNT(*) AS n FROM friendships WHERE status='accepted'")
    local pending  = cero.queryrow("SELECT COUNT(*) AS n FROM friendships WHERE status='pending'")
    local messages = cero.queryrow("SELECT COUNT(*) AS n FROM messages")
    local unread   = cero.queryrow("SELECT COUNT(*) AS n FROM messages WHERE read_at IS NULL")

    local online = 0
    local hub = cero.hub()
    if hub then online = hub.online end

    cero.out(string.format([[
  uptime      %s
  online      %d
  users       %d
  friends     %d accepted / %d pending
  messages    %d (%d unread)
]], cero.uptime(), online, users.n, accepted.n, pending.n, messages.n, unread.n))
end)