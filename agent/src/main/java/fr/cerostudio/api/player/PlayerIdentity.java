package fr.cerostudio.api.player;

import java.nio.charset.StandardCharsets;
import java.util.UUID;

public final class PlayerIdentity {

    private final String username;
    private final UUID uuid;

    private PlayerIdentity(String username, UUID uuid) {
        this.username = username;
        this.uuid = uuid;
    }

    public String getUsername() {
        return username;
    }

    public UUID getUuid() {
        return uuid;
    }

    public static PlayerIdentity resolve(String username, String rawUuid) {
        String resolvedUsername = (username != null && !username.isEmpty()) ? username : "Player";
        UUID uuid = parseUuid(rawUuid);
        if (uuid == null) {
            uuid = offlineUuid(resolvedUsername);
        }
        return new PlayerIdentity(resolvedUsername, uuid);
    }

    private static UUID parseUuid(String raw) {
        if (raw == null || raw.isEmpty()) return null;
        try {
            String normalized = raw;
            if (!raw.contains("-") && raw.length() == 32) {
                normalized = raw.replaceFirst(
                        "(\\w{8})(\\w{4})(\\w{4})(\\w{4})(\\w{12})", "$1-$2-$3-$4-$5");
            }
            return UUID.fromString(normalized);
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    private static UUID offlineUuid(String username) {
        return UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(StandardCharsets.UTF_8));
    }

    @Override
    public String toString() {
        return username + " (" + uuid + ")";
    }
}
