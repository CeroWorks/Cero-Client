package fr.cerostudio.api.event;

public abstract class Event {

    private boolean cancelled = false;

    public boolean isCancelable() {
        return false;
    }

    public final boolean isCancelled() {
        return cancelled;
    }

    public final void setCancelled(boolean cancelled) {
        if (!isCancelable()) {
            throw new UnsupportedOperationException(getClass().getSimpleName() + " n'est pas annulable");
        }
        this.cancelled = cancelled;
    }
}
