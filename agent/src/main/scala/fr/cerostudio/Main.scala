package fr.cerostudio

import java.io.File
import java.net.{URL, URLClassLoader}
import java.lang.reflect.Method
import java.util.regex.Pattern
import scala.annotation.tailrec
import scala.collection.mutable
import scala.jdk.CollectionConverters._
import scala.util.{Try, Success, Failure}

object Main {

  case class LaunchConfig(
    realMainClass: String,
    mcVersion: String,
    debug: Boolean = false,
    forceFlavor: Option[VersionSupport.McFlavor] = None
  )

  def main(args: Array[String]): Unit = {
    val (config, forwardedArgs) = parseArgs(args.toList, LaunchConfig(null, "unknown"), Nil)

    if (config.realMainClass == null) {
      System.err.println("[CeroClient] --realMainClass manquant, abandon.")
      System.exit(1)
    }

    val mcVersion = VersionSupport.parse(config.mcVersion)

    config.forceFlavor match {
      case Some(f) =>
        System.out.println(
          s"[CeroClient] Démarrage — Version = ${config.mcVersion} | " +
          s"Flavor forcé = ${flavorName(f)} | mainClass = ${config.realMainClass}"
        )
      case None =>
        System.out.println(
          s"[CeroClient] Démarrage — Version = ${config.mcVersion} " +
          s"(${flavorName(mcVersion.flavor)}) | mainClass = ${config.realMainClass}"
        )
    }

    if (config.debug) {
      System.out.println(s"[CeroClient][DEBUG] Args transférés: ${forwardedArgs.mkString("[", ", ", "]")}")
    }

    val classpathUrls = extractClasspathUrls()

    val rawClassPath = System.getProperty("java.class.path", "<vide>")
    System.out.println(s"[CeroClient][DIAG] java.class.path brut = $rawClassPath")
    System.out.println(s"[CeroClient][DIAG] ${classpathUrls.length} URL(s) de classpath résolue(s):")
    classpathUrls.foreach(u => System.out.println(s"[CeroClient][DIAG]   - $u"))
    val clientJarUrl = classpathUrls.find(_.toString.contains("client.jar"))
    clientJarUrl match {
      case Some(u) => System.out.println(s"[CeroClient][DIAG] client.jar trouvé dans classpathUrls: $u")
      case None    => System.err.println("[CeroClient][DIAG] AUCUNE URL contenant 'client.jar' trouvée dans classpathUrls !")
    }

    val remapper = new RemappingClassLoader(
      classpathUrls,
      getClass.getClassLoader.getParent,
      mcVersion,
      config.forceFlavor,
      config.debug
    )

    Thread.currentThread().setContextClassLoader(remapper)

    launchMain(config.realMainClass, forwardedArgs.toArray, remapper) match {
      case Success(_) =>
      case Failure(e) =>
        System.err.println(s"[CeroClient] Erreur lors du lancement de ${config.realMainClass}")
        e.printStackTrace()
        System.exit(1)
    }
  }

  @tailrec
  private def parseArgs(args: List[String], config: LaunchConfig, acc: List[String]): (LaunchConfig, List[String]) = args match {
    case Nil => (config, acc.reverse)

    case "--realMainClass" :: value :: rest =>
      parseArgs(rest, config.copy(realMainClass = value), acc)

    case "--ceroMcVersion" :: value :: rest =>
      parseArgs(rest, config.copy(mcVersion = value), acc)

    case "--ceroDebug" :: rest =>
      parseArgs(rest, config.copy(debug = true), acc)

    case "--ceroLegacy" :: rest =>
      parseArgs(rest, config.copy(forceFlavor = Some(VersionSupport.Legacy)), acc)

    case "--ceroModern" :: rest =>
      parseArgs(rest, config.copy(forceFlavor = Some(VersionSupport.Modern)), acc)

    case arg :: rest =>
      parseArgs(rest, config, arg :: acc)
  }

  private def extractClasspathUrls(): Array[URL] = {
    val cp   = System.getProperty("java.class.path", "")
    val sep  = System.getProperty("path.separator", ":")
    val parts = cp.split(Pattern.quote(sep))

    parts.iterator.flatMap { p =>
      Try(new File(p).toURI.toURL).toOption
    }.toArray
  }

  private def launchMain(mainClassName: String, args: Array[String], cl: ClassLoader): Try[Unit] = Try {
    val mainClass  = Class.forName(mainClassName, true, cl)
    val mainMethod = mainClass.getMethod("main", classOf[Array[String]])
    mainMethod.setAccessible(true)
    mainMethod.invoke(null, args.asInstanceOf[AnyRef])
  }

  private def flavorName(flavor: VersionSupport.McFlavor): String = flavor match {
    case VersionSupport.Legacy  => "Legacy (LWJGL 2)"
    case VersionSupport.Modern  => "Modern (LWJGL 3)"
    case VersionSupport.Unknown => "Unknown"
  }
}
