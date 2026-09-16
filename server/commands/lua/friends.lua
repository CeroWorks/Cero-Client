local cero = require("cero")

cero.define("friends", "list friends of a user — friends <name|uuid>", function(name)
    if not name then
        cero.out("usage: friends <name|uuid>")
        return
    end
    local u = cero.finduser(name)
    if not u then
        cero.out(string.format("user %q not found", name))
        return
    end

    local rows, err = cero.query([[
        SELECT CASE WHEN f.user_a = ? THEN f.user_b ELSE f.user_a END AS uuid,
               CASE WHEN f.user_a = ? THEN ub.username ELSE ua.username END AS name,
               CASE WHEN f.user_a = ? THEN ub.status   ELSE ua.status   END AS status,
               f.created_at AS since
        FROM friendships f
        JOIN users ua ON ua.uuid = f.user_a
        JOIN users ub ON ub.uuid = f.user_b
        WHERE (f.user_a = ? OR f.user_b = ?) AND f.status = 'accepted'
    ]], u.uuid, u.uuid, u.uuid, u.uuid, u.uuid)

    if not rows then
        cero.out("db error: " .. (err or "?"))
        return
    end

    cero.out("  friends of " .. u.username .. ":")
    local count = 0
    for _, row in ipairs(rows) do
        local since = os.date("%Y-%m-%d", row.since / 1000)
        cero.out(string.format("  %-17s %-9s (since %s)", row.name, row.status, since))
        count = count + 1
    end
    if count == 0 then
        cero.out("  none")
    end
end)