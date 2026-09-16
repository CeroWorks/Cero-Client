local cero = require("cero")

cero.define("online", "list connected users (ws sessions)", function()
    local hub = cero.hub()
    if not hub then
        cero.out("hub unavailable")
        return
    end
    local snap = hub.snapshot()
    local count = 0
    for _ in pairs(snap) do count = count + 1 end
    if count == 0 then
        cero.out("  nobody online")
        return
    end
    cero.out(string.format("  %-17s %-9s %s", "USERNAME", "SESSIONS", "UUID"))
    for uuid, n in pairs(snap) do
        local u = cero.queryrow("SELECT username FROM users WHERE uuid = ?", uuid)
        local name = u and u.username or "?"
        cero.out(string.format("  %-17s %-9d %s", name, n, uuid))
    end
end)