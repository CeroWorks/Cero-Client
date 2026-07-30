package fr.cerostudio.loader

import org.spongepowered.asm.mixin.Mixins
import java.lang.instrument.Instrumentation

/**
 * Java Agent premain entry point pour le mode instrumentation.
 * Alternative au ClassLoader-based mixin loading.
 *
 * En Scala mais doit conserver la signature exacte attendue par JVM :
 *   public static void premain(String, Instrumentation)
 */
object CeroLoader {

  def premain(args: String, inst: Instrumentation): Unit = {
    System.out.println("[CeroClient] Starting Mixin (agent mode)...")
    Mixins.addConfiguration("mixins.cero.json")
    System.out.println("[CeroClient] Mixin agent initialized !")
  }
}
