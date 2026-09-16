package fr.cerostudio.mixin;

import fr.cerostudio.api.CeroApi;
import fr.cerostudio.api.event.client.ClientTickEvent;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

public final class MixinClientTick {

    private MixinClientTick() {}

    @Mixin(targets = "net.minecraft.client.Minecraft")
    public static class Legacy {

        @Inject(method = "func_71407_l", at = @At("HEAD"))
        private void cero$onTickStart(CallbackInfo ci) {
            CeroApi.events().post(new ClientTickEvent(ClientTickEvent.Phase.START));
        }

        @Inject(method = "func_71407_l", at = @At("RETURN"))
        private void cero$onTickEnd(CallbackInfo ci) {
            CeroApi.events().post(new ClientTickEvent(ClientTickEvent.Phase.END));
        }
    }

    @Mixin(targets = "net.minecraft.client.Minecraft")
    public static class Modern {

        @Inject(method = "tick", at = @At("HEAD"))
        private void cero$onTickStart(CallbackInfo ci) {
            CeroApi.events().post(new ClientTickEvent(ClientTickEvent.Phase.START));
        }

        @Inject(method = "tick", at = @At("RETURN"))
        private void cero$onTickEnd(CallbackInfo ci) {
            CeroApi.events().post(new ClientTickEvent(ClientTickEvent.Phase.END));
        }
    }
}
