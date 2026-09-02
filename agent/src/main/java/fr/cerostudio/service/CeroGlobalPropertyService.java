package fr.cerostudio.service;

import org.spongepowered.asm.service.IGlobalPropertyService;
import org.spongepowered.asm.service.IPropertyKey;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class CeroGlobalPropertyService implements IGlobalPropertyService {

    private final Map<String, Object> properties = new ConcurrentHashMap<>();

    private static final class Key implements IPropertyKey {
        final String name;
        Key(String name) { this.name = name; }

        @Override
        public boolean equals(Object o) {
            return o instanceof Key && ((Key) o).name.equals(this.name);
        }

        @Override
        public int hashCode() { return name.hashCode(); }

        @Override
        public String toString() { return name; }
    }

    @Override
    public IPropertyKey resolveKey(String name) {
        return new Key(name);
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T getProperty(IPropertyKey key) {
        return (T) properties.get(((Key) key).name);
    }

    @Override
    public void setProperty(IPropertyKey key, Object value) {
        properties.put(((Key) key).name, value);
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T getProperty(IPropertyKey key, T defaultValue) {
        return (T) properties.getOrDefault(((Key) key).name, defaultValue);
    }

    @Override
    public String getPropertyString(IPropertyKey key, String defaultValue) {
        Object value = properties.get(((Key) key).name);
        return value != null ? value.toString() : defaultValue;
    }
}