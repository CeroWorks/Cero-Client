package fr.cerostudio.service

import fr.cerostudio.RemappingClassLoader
import org.objectweb.asm.{ClassReader => AsmClassReader}
import org.objectweb.asm.tree.ClassNode
import org.spongepowered.asm.launch.platform.container.IContainerHandle
import org.spongepowered.asm.launch.platform.container.ContainerHandleVirtual
import org.spongepowered.asm.logging.{ILogger, Level}
import org.spongepowered.asm.mixin.MixinEnvironment
import org.spongepowered.asm.mixin.transformer.{IMixinTransformer, IMixinTransformerFactory}
import org.spongepowered.asm.service._
import org.spongepowered.asm.util.ReEntranceLock

import java.io.{IOException, InputStream}
import java.net.URL
import scala.jdk.CollectionConverters._
import scala.util.{Failure, Success, Try, Using}

/**
 * Implémentation Scala complète du service Mixin pour le mode ClassLoader.
 *
 * Implémente plusieurs interfaces Mixin en un seul objet Scala grâce au
 * pattern « trait forwarding ». C'est plus concis que l'équivalent Java
 * avec 7 interfaces séparées.
 */
class CeroMixinService
    extends IMixinService
    with IClassProvider
    with IClassBytecodeProvider
    with ITransformerProvider
    with IClassTracker
    with IMixinAuditTrail {

  private val lock = new ReEntranceLock(1)
  private var transformerFactory: IMixinTransformerFactory = _
  private var transformer: IMixinTransformer = _
  private val primaryContainer: IContainerHandle =
    new ContainerHandleVirtual("CeroClient")

  // ── IMixinService ───────────────────────────────────────────────────

  override def getName: String = "Cero ClassLoader Service (Scala)"

  override def isValid: Boolean = true

  override def prepare(): Unit = ()

  override def getInitialPhase: MixinEnvironment.Phase = MixinEnvironment.Phase.DEFAULT

  override def offer(internal: IMixinInternal): Unit = internal match {
    case factory: IMixinTransformerFactory =>
      this.transformerFactory = factory
    case _ =>
  }

  /**
   * Crée le transformer Mixin une seule fois (lazy init).
   */
  def getTransformer: IMixinTransformer = {
    if (transformer == null && transformerFactory != null) {
      Try(transformerFactory.createTransformer()) match {
        case Success(t) => transformer = t
        case Failure(e) =>
          System.err.println("[CeroMixinService] Erreur création transformer:")
          e.printStackTrace()
      }
    }
    transformer
  }

  override def init(): Unit = ()

  override def beginPhase(): Unit = ()

  override def checkEnv(bootSource: Any): Unit = ()

  override def getReEntranceLock: ReEntranceLock = lock

  override def getClassProvider: IClassProvider = this

  override def getBytecodeProvider: IClassBytecodeProvider = this

  override def getTransformerProvider: ITransformerProvider = this

  override def getClassTracker: IClassTracker = this

  override def getAuditTrail: IMixinAuditTrail = this

  override def getPlatformAgents: java.util.Collection[String] = java.util.Collections.emptyList()

  override def getPrimaryContainer: IContainerHandle = primaryContainer

  override def getMixinContainers: java.util.Collection[IContainerHandle] = java.util.Collections.emptyList()

  override def getSideName: String = "CLIENT"

  override def getMinCompatibilityLevel: MixinEnvironment.CompatibilityLevel =
    MixinEnvironment.CompatibilityLevel.JAVA_8

  override def getMaxCompatibilityLevel: MixinEnvironment.CompatibilityLevel =
    MixinEnvironment.CompatibilityLevel.JAVA_17

  // ── Logging ────────────────────────────────────────────────────────

  override def getLogger(name: String): ILogger = new CeroLogger(name)

  // ── IClassProvider ─────────────────────────────────────────────────

  override def getResourceAsStream(name: String): InputStream =
    getContextClassLoader.getResourceAsStream(name)

  override def getClassPath: Array[URL] = Array.empty

  override def findClass(name: String): Class[_] =
    Class.forName(name, true, getContextClassLoader)

  override def findClass(name: String, initialize: Boolean): Class[_] =
    Class.forName(name, initialize, getContextClassLoader)

  override def findAgentClass(name: String, initialize: Boolean): Class[_] =
    Class.forName(name, initialize, getContextClassLoader)

  // ── IClassBytecodeProvider ─────────────────────────────────────────

  override def getClassNode(name: String): ClassNode = {
    getContextClassLoader match {
      case remapper: RemappingClassLoader =>
        val bytes = remapper.readResourceBytes(name.replace('.', '/') + ".class")
        if (bytes == null) throw new ClassNotFoundException(name)
        val node = new ClassNode()
        new AsmClassReader(bytes).accept(node, 0)
        node

      case other =>
        throw new ClassNotFoundException(
          s"CeroMixinService cannot load outside RemappingClassLoader (${other.getClass.getName}): $name"
        )
    }
  }

  override def getClassNode(name: String, runTransformers: Boolean): ClassNode =
    getClassNode(name)

  // ── ITransformerProvider ────────────────────────────────────────────

  override def getTransformers: java.util.Collection[ITransformer] = java.util.Collections.emptyList()

  override def getDelegatedTransformers: java.util.Collection[ITransformer] = java.util.Collections.emptyList()

  override def addTransformerExclusion(name: String): Unit = ()

  // ── IClassTracker ───────────────────────────────────────────────────

  override def registerInvalidClass(className: String): Unit = ()

  override def isClassLoaded(className: String): Boolean = false

  override def getClassRestrictions(className: String): String = ""

  // ── IMixinAuditTrail ──────────────────────────────────────────────

  override def onApply(className: String, mixinName: String): Unit = ()

  override def onPostProcess(className: String): Unit = ()

  override def onGenerate(className: String, proxyName: String): Unit = ()

  // ── Utilitaires ────────────────────────────────────────────────────

  private def getContextClassLoader: ClassLoader =
    Thread.currentThread().getContextClassLoader
}

/**
 * Logger Mixin simplifié en Scala. Utilise le pattern matching sur le Level
 * pour un code concis et lisible.
 */
class CeroLogger(loggerName: String) extends ILogger {

  private def prefix(level: String): String = s"[$level] [$loggerName] "

  private def logOut(level: String, message: String, params: AnyRef*): Unit =
    System.out.printf(prefix(level) + message + "%n", params: _*)

  private def logErr(level: String, message: String): Unit =
    System.err.println(prefix(level) + message)

  private def logErrThrowable(level: String, message: String, t: Throwable): Unit = {
    System.err.println(prefix(level) + message)
    t.printStackTrace()
  }

  override def getId: String       = "cero"
  override def getType: String     = "CeroLogger"

  override def catching(level: Level, t: Throwable): Unit = t.printStackTrace()
  override def catching(t: Throwable): Unit               = t.printStackTrace()

  override def debug(message: String, params: AnyRef*): Unit               = logOut("DEBUG", message, params: _*)
  override def debug(message: String, t: Throwable): Unit                 = logErrThrowable("DEBUG", message, t)

  override def error(message: String, params: AnyRef*): Unit               = logOut("ERROR", message, params: _*)
  override def error(message: String, t: Throwable): Unit                 = logErrThrowable("ERROR", message, t)

  override def fatal(message: String, params: AnyRef*): Unit               = logOut("FATAL", message, params: _*)
  override def fatal(message: String, t: Throwable): Unit                 = logErrThrowable("FATAL", message, t)

  override def info(message: String, params: AnyRef*): Unit                = logOut("INFO", message, params: _*)
  override def info(message: String, t: Throwable): Unit                  = logErrThrowable("INFO", message, t)

  override def log(level: Level, message: String, params: AnyRef*): Unit   = logOut(level.name(), message, params: _*)
  override def log(level: Level, message: String, t: Throwable): Unit     = logErrThrowable(level.name(), message, t)

  override def throwing[T <: Throwable](t: T): T = t

  override def trace(message: String, params: AnyRef*): Unit               = logOut("TRACE", message, params: _*)
  override def trace(message: String, t: Throwable): Unit                 = logErrThrowable("TRACE", message, t)

  override def warn(message: String, params: AnyRef*): Unit                = logOut("WARN", message, params: _*)
  override def warn(message: String, t: Throwable): Unit                  = logErrThrowable("WARN", message, t)
}
