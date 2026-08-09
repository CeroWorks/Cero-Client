package fr.cerostudio.mixin.v1;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArg;

@Mixin(targets = "net.minecraft.client.Minecraft")
public class MixinLegacyTitle {

    @ModifyArg(
        method = "func_71384_a|func_175609_am",
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
