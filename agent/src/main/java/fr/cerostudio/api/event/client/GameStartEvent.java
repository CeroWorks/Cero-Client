package fr.cerostudio.api.event.client;

import fr.cerostudio.api.event.Event;

public final class GameStartEvent extends Event {

    private final String mcVersion;

    public GameStartEvent(String mcVersion) {
        this.mcVersion = mcVersion;
    }

    public String getMcVersion() {
        return mcVersion;
    }
}
