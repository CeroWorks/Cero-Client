package fr.cerostudio.api;

import fr.cerostudio.api.capability.CapabilitySet;
import fr.cerostudio.api.event.EventBus;
import fr.cerostudio.api.mod.ModLoader;
import fr.cerostudio.api.player.PlayerIdentity;
import fr.cerostudio.api.service.ServiceRegistry;
import fr.cerostudio.api.window.WindowApi;

public final class CeroApi {

    private static EventBus eventBus;
    private static ServiceRegistry serviceRegistry;
    private static ModLoader modLoader;
    private static CapabilitySet capabilitySet;
    private static PlayerIdentity playerIdentity;
    private static WindowApi windowApi;

    private CeroApi() {}

    public static void bootstrap(CapabilitySet capabilities, ModLoader loader, PlayerIdentity identity) {

        System.out.println("[CeroApi] bootstrap() par CL=" + CeroApi.class.getClassLoader());

        if (eventBus != null) {
            throw new IllegalStateException("CeroApi déjà initialisée");
        }
        capabilitySet = capabilities;
        eventBus = new EventBus();
        serviceRegistry = new ServiceRegistry();
        modLoader = loader;
        playerIdentity = identity;
        windowApi = new WindowApi();
    }

    public static EventBus events() {
        return require(eventBus, "EventBus");
    }

    public static ServiceRegistry services() {
        return require(serviceRegistry, "ServiceRegistry");
    }

    public static ModLoader mods() {
        return require(modLoader, "ModLoader");
    }

    public static CapabilitySet capabilities() {
        return require(capabilitySet, "CapabilitySet");
    }

    public static PlayerIdentity player() {
        return require(playerIdentity, "PlayerIdentity");
    }

    public static WindowApi window() {
        return require(windowApi, "WindowApi");
    }

    public static String minecraftVersion() {
        return capabilities().mcVersion();
    }

    private static <T> T require(T value, String name) {
        if (value == null) {
            System.out.println("[CeroApi] require(" + name + ") échoue, CL=" + CeroApi.class.getClassLoader());
            throw new IllegalStateException(
                    name + " demandé avant CeroApi.bootstrap() — appel trop tôt dans le cycle de vie ?");
        }
        return value;
    }
}