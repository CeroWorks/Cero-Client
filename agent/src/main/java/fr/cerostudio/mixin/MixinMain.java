package fr.cerostudio.mixin;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Mixin injecté dans le main de Minecraft.
 *
 * ⚠ IMPORTANT: Cette classe DOIT rester en Java car SpongePowered Mixin
 * nécessite des patterns bytecode Java spécifiques que Scala ne peut pas
 * reproduire correctement.
 */
@Mixin(targets = "net.minecraft.client.main.Main")
public class MixinMain {

    @Inject(method = "main", at = @At("HEAD"))
    private static void onMain(String[] args, CallbackInfo ci) {
        System.out.println("[CeroClient] Hello from Mixin ! Le client est injecté.");
    }
}
