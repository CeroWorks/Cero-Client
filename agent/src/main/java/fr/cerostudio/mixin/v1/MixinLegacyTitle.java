package fr.cerostudio.mixin.v1;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArg;

/**
 * Mixin Legacy — Modifie le titre de la fenêtre pour les versions 1.7.10 → 1.12.2.
 * Utilise Display.setTitle() via LWJGL 2.
 *
 * ⚠ IMPORTANT: Cette classe DOIT rester en Java (voir MixinMain).
 */
@Mixin(targets = "net.minecraft.client.Minecraft")
public class MixinLegacyTitle {

    @ModifyArg(
        method = "func_71384_a|func_175609_am", // Couvre 1.7.10, 1.8.9 ET 1.12.2
        at = @At(
            value = "INVOKE",
            target = "Lorg/lwjgl/opengl/Display;setTitle(Ljava/lang/String;)V"
        ),
        index = 0
    )
    private String cero$modifyLegacyTitle(String originalTitle) {
        return "CeroClient | Legacy Edition";
    }
}
