local cero = require("cero")

cero.define("kick", "disconnect a user — kick <name|uuid>", function(name)
    if not name then
        cero.out("usage: kick <name|uuid>")
        return
    end
    local u = cero.finduser(name)
    if not u then
        cero.out(string.format("user %q not found", name))
        return
    end
    local hub = cero.hub()
    local n = hub and hub.kick(u.uuid) or 0
    cero.out(string.format("kicked %q — %d session(s) closed", u.username, n))
end)