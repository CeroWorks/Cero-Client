package fr.cerostudio.core;

import fr.cerostudio.api.CeroApi;
import fr.cerostudio.api.mod.ClientModInitializer;

public final class CeroClientMod implements ClientModInitializer {

    @Override
    public void onInitializeClient() {
        String version = CeroApi.minecraftVersion();
        String pseudo = CeroApi.player() != null ? CeroApi.player().getUsername() : "Player";
        CeroApi.window().setTitle("CeroClient " + version + " " + pseudo);
    }
}