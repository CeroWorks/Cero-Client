package fr.cerostudio.api.mod;

public final class ModContainer {

    private final String id;
    private final String name;
    private final ModInitializer mainInitializer;
    private final ClientModInitializer clientInitializer;

    public ModContainer(String id, String name, ModInitializer mainInitializer, ClientModInitializer clientInitializer) {
        this.id = id;
        this.name = name;
        this.mainInitializer = mainInitializer;
        this.clientInitializer = clientInitializer;
    }

    public static ModContainer clientOnly(String id, String name, ClientModInitializer clientInitializer) {
        return new ModContainer(id, name, null, clientInitializer);
    }

    public String id() {
        return id;
    }

    public String name() {
        return name;
    }

    public ModInitializer mainInitializer() {
        return mainInitializer;
    }

    public ClientModInitializer clientInitializer() {
        return clientInitializer;
    }
}
