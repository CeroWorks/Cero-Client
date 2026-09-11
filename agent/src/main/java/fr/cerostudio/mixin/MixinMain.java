package fr.cerostudio.mixin;

import fr.cerostudio.api.CeroApi;
import fr.cerostudio.api.event.client.GameStartEvent;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(targets = "net.minecraft.client.main.Main")
public class MixinMain {

    @Inject(method = "main", at = @At("HEAD"))
    private static void onMain(String[] args, CallbackInfo ci) {
        CeroApi.events().post(new GameStartEvent(CeroApi.capabilities().mcVersion()));
    }
}