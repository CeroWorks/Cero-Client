package fr.cerostudio.api.event.client;

import fr.cerostudio.api.event.Event;

public final class ClientTickEvent extends Event {

    public enum Phase {
        START,
        END
    }

    private final Phase phase;

    public ClientTickEvent(Phase phase) {
        this.phase = phase;
    }

    public Phase getPhase() {
        return phase;
    }
}
