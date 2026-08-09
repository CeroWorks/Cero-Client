package fr.cerostudio.loader

import org.spongepowered.asm.mixin.Mixins
import java.lang.instrument.Instrumentation

object CeroLoader {

  def premain(args: String, inst: Instrumentation): Unit = {
    System.out.println("[CeroClient] Starting Mixin (agent mode)...")
    Mixins.addConfiguration("mixins.cero.json")
    System.out.println("[CeroClient] Mixin agent initialized !")
  }
}
