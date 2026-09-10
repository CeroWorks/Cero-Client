package fr.cerostudio.api.service;

import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

public final class ServiceRegistry {

    private final Map<Class<?>, Object> services = new ConcurrentHashMap<>();

    public <T> void register(Class<T> type, T implementation) {
        if (implementation == null) {
            throw new IllegalArgumentException("Impossible d'enregistrer un service null pour " + type.getName());
        }
        services.put(type, implementation);
    }

    public <T> Optional<T> find(Class<T> type) {
        return Optional.ofNullable(type.cast(services.get(type)));
    }

    public <T> T get(Class<T> type) {
        return find(type).orElseThrow(() ->
                new IllegalStateException("Aucun service enregistré pour " + type.getName()));
    }

    public boolean isRegistered(Class<?> type) {
        return services.containsKey(type);
    }
}
