package fr.cerostudio.loader;

import org.spongepowered.asm.mixin.Mixins;

import java.lang.instrument.Instrumentation;

public class CeroLoader {

    public static void premain(String args, Instrumentation inst) {
        System.out.println("[CeroClient] Starting Mixin...");

        Mixins.addConfiguration("mixins.cero.json");

        System.out.println("[CeroClient] Finished !");
    }
}