package fr.cerostudio

import fr.cerostudio.service.CeroMixinService
import org.spongepowered.asm.launch.MixinBootstrap
import org.spongepowered.asm.mixin.Mixins
import org.spongepowered.asm.mixin.transformer.IMixinTransformer
import org.spongepowered.asm.service.MixinService

import java.io.{ByteArrayOutputStream, IOException, InputStream}
import java.net.{URL, URLClassLoader}
import scala.util.{Try, Using}

class RemappingClassLoader(
  urls: Array[URL],
  parent: ClassLoader,
  mcVersion: VersionSupport.McVersion,
  forceFlavor: Option[VersionSupport.McFlavor] = None,
  debug: Boolean = false
) extends URLClassLoader(urls, parent) {

  private var mixinTransformer: IMixinTransformer = _

  initMixin()

  private def initMixin(): Unit = {
    val flavorLabel = forceFlavor match {
      case Some(f) => flavorName(f) + " (forcé)"
      case None    => flavorName(mcVersion.flavor)
    }

    System.out.println(
      s"[CeroClassLoader] Démarrage de Mixin en mode ClassLoader pour ${mcVersion.raw} ($flavorLabel)..."
    )

    Thread.currentThread().setContextClassLoader(this)

    MixinBootstrap.init()

    val mixinConfigs = forceFlavor match {
      case Some(f) =>
        List(f match {
          case VersionSupport.Legacy  => "mixins.cero.v1_legacy.json"
          case VersionSupport.Modern  => "mixins.cero.v1_modern.json"
          case VersionSupport.Unknown => "mixins.cero.v1_modern.json"
        })
      case None =>
        VersionSupport.mixinConfigs(mcVersion)
    }

    mixinConfigs.foreach(Mixins.addConfiguration)

    MixinService.getService() match {
      case service: CeroMixinService =>
        this.mixinTransformer = service.getTransformer
        if (this.mixinTransformer != null) {
          System.out.println("[CeroClassLoader] Moteur Mixin hooké avec succès !")
        } else {
          System.err.println("[CeroClassLoader] Erreur: transformer Mixin indisponible.")
        }

      case other =>
        System.err.println(
          s"[CeroClassLoader] Service Mixin inattendu: ${other.getClass.getName}"
        )
    }
  }

  override protected def loadClass(name: String, resolve: Boolean): Class[_] = {
    getClassLoadingLock(name).synchronized {
      val loaded = findLoadedClass(name)
      if (loaded != null) return loaded

      if (shouldLoadLocally(name)) {
        tryLocallyThenParent(name, resolve)
      } else {
        tryParentThenLocally(name, resolve)
      }
    }
  }

  override protected def findClass(name: String): Class[_] = {
    val path = name.replace('.', '/') + ".class"

    Using.Manager { use =>
      val isOpt = Option(use(this.getResourceAsStream(path)))
      isOpt match {
        case None => throw new ClassNotFoundException(name)

        case Some(is) =>
          val rawBytes = readAllBytes(is)
          val transformedBytes = transformWithMixin(name, rawBytes)

          definePackageIfNeeded(name)

          val clazz = defineClass(name, transformedBytes, 0, transformedBytes.length)
          resolveClass(clazz)
          clazz
      }
    }.fold(
      {
        case e: ClassNotFoundException => throw e
        case e: IOException => throw new ClassNotFoundException(name, e)
        case e => throw new ClassNotFoundException(name, e)
      },
      identity
    )
  }

  def readResourceBytes(path: String): Array[Byte] = {
    Option(this.getResourceAsStream(path)) match {
      case None =>
        System.err.println(s"[CeroClassLoader][DIAG] Resource introuvable: $path")
        System.err.println(s"[CeroClassLoader][DIAG] URLs connues de ce loader (${this.getURLs.length}):")
        this.getURLs.foreach(u => System.err.println(s"[CeroClassLoader][DIAG]   - $u"))
        val viaFind = Option(this.findResource(path))
        System.err.println(s"[CeroClassLoader][DIAG] findResource direct: $viaFind")
        null
      case Some(is) =>
        Using.resource(is)(readAllBytes)
    }
  }

  private def transformWithMixin(name: String, bytes: Array[Byte]): Array[Byte] = {
    if (name.startsWith("fr.cerostudio.")) return bytes

    if (mixinTransformer != null) {
      Try {
        val result = mixinTransformer.transformClassBytes(name, name, bytes)
        Option(result).getOrElse(bytes)
      }.fold(
        { e =>
          System.err.println(s"[CeroClassLoader] Erreur de transformation Mixin pour $name")
          e.printStackTrace()
          bytes
        },
        identity
      )
    } else {
      bytes
    }
  }

  private def shouldLoadLocally(name: String): Boolean = {
    name.startsWith("net.minecraft.") ||
    name.startsWith("com.mojang.") ||
    name.startsWith("fr.cerostudio.")
  }

  private def tryLocallyThenParent(name: String, resolve: Boolean): Class[_] = {
    try {
      val c = findClass(name)
      if (resolve) resolveClass(c)
      c
    } catch {
      case _: ClassNotFoundException => getParent.loadClass(name)
    }
  }

  private def tryParentThenLocally(name: String, resolve: Boolean): Class[_] = {
    try {
      getParent.loadClass(name)
    } catch {
      case _: ClassNotFoundException =>
        val c = findClass(name)
        if (resolve) resolveClass(c)
        c
    }
  }

  private def readAllBytes(is: InputStream): Array[Byte] = {
    val bos = new ByteArrayOutputStream()
    val buf = new Array[Byte](8192)
    var n = is.read(buf)
    while (n != -1) {
      bos.write(buf, 0, n)
      n = is.read(buf)
    }
    bos.toByteArray
  }

  private def definePackageIfNeeded(name: String): Unit = {
    val lastDot = name.lastIndexOf('.')
    if (lastDot != -1) {
      val packageName = name.substring(0, lastDot)
      try {
        val method = classOf[ClassLoader].getMethod("getDefinedPackage", classOf[String])
        if (method.invoke(this, packageName) == null) {
          definePackage(packageName, null, null, null, null, null, null, null)
        }
      } catch {
        case _: NoSuchMethodException =>
          if (getPackage(packageName) == null) {
            definePackage(packageName, null, null, null, null, null, null, null)
          }
      }
    }
  }

  private def flavorName(flavor: VersionSupport.McFlavor): String = flavor match {
    case VersionSupport.Legacy  => "Legacy (LWJGL 2)"
    case VersionSupport.Modern  => "Modern (LWJGL 3)"
    case VersionSupport.Unknown => "Unknown"
  }
}
