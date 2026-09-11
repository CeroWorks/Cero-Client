package fr.cerostudio.api.mod;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class ModLoader {

    private final List<ModContainer> loaded = new ArrayList<>();

    public void load(ModContainer mod) {
        try {
            if (mod.mainInitializer() != null) {
                mod.mainInitializer().onInitialize();
            }
            if (mod.clientInitializer() != null) {
                mod.clientInitializer().onInitializeClient();
            }
            loaded.add(mod);
            System.out.println("[CeroApi] Mod chargé: " + mod.name() + " (" + mod.id() + ")");
        } catch (Exception e) {
            System.err.println("[CeroApi] Échec du chargement du mod '" + mod.id() + "': " + e);
        }
    }

    public List<ModContainer> getLoadedMods() {
        return Collections.unmodifiableList(loaded);
    }
}
