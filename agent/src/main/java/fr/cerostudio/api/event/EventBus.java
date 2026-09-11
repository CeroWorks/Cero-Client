package fr.cerostudio.api.event;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.Consumer;

public final class EventBus {

    private static final class Registration<T extends Event> {
        final EventPriority priority;
        final Consumer<T> listener;

        Registration(EventPriority priority, Consumer<T> listener) {
            this.priority = priority;
            this.listener = listener;
        }
    }

    private final ConcurrentHashMap<Class<? extends Event>, List<Registration<?>>> listeners = new ConcurrentHashMap<>();

    public <T extends Event> void register(Class<T> eventType, EventPriority priority, Consumer<T> listener) {
        listeners
                .computeIfAbsent(eventType, k -> new CopyOnWriteArrayList<>())
                .add(new Registration<>(priority, listener));
    }

    public <T extends Event> void register(Class<T> eventType, Consumer<T> listener) {
        register(eventType, EventPriority.NORMAL, listener);
    }

    @SuppressWarnings("unchecked")
    public <T extends Event> T post(T event) {
        List<Registration<?>> registrations = listeners.get(event.getClass());
        if (registrations == null || registrations.isEmpty()) {
            return event;
        }

        List<Registration<?>> ordered = new ArrayList<>(registrations);
        ordered.sort(Comparator.comparing(r -> r.priority));

        for (Registration<?> registration : ordered) {
            if (event.isCancelable() && event.isCancelled()) {
                break;
            }
            ((Consumer<T>) registration.listener).accept(event);
        }
        return event;
    }
}
