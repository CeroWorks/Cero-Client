package fr.cerostudio.mixin.v2;

import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArg;

@Mixin(targets = "com.mojang.blaze3d.platform.Window")
public class MixinModernTitle {

    @ModifyArg(
            method = "<init>",
            at = @At(
                    value = "INVOKE",
                    target = "Lorg/lwjgl/glfw/GLFW;glfwCreateWindow(IILjava/lang/CharSequence;JJ)J"
            ),
            index = 2
    )
    private CharSequence cero$modifyCreateWindowTitle(CharSequence originalTitle) {
        return "CeroClient | Modern Edition";
    }

    @ModifyArg(
            method = "setTitle",
            at = @At(
                    value = "INVOKE",
                    target = "Lorg/lwjgl/glfw/GLFW;glfwSetWindowTitle(JLjava/lang/CharSequence;)V"
            ),
            index = 1
    )
    private CharSequence cero$modifyModernTitle(CharSequence originalTitle) {
        return "CeroClient | Modern Edition";
    }
}