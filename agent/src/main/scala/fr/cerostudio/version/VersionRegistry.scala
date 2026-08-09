package fr.cerostudio.version

import fr.cerostudio.VersionSupport
import fr.cerostudio.VersionSupport.McVersion
import fr.cerostudio.version.VersionManifest.{VersionManifest => VManifest, LogicalTarget}

import java.io.File
import java.net.{URI, URL}
import java.nio.file.{Files, Path, Paths}
import scala.jdk.CollectionConverters._
import scala.util.{Failure, Success, Try}

object VersionRegistry {

  private var manifests: Map[String, VManifest] = Map.empty
  private var initialized = false

  def init(externalDir: Option[Path] = None): Unit = synchronized {
    if (initialized) return

    val fromExternal = externalDir.map(loadFromDirectory).getOrElse(Map.empty)
    val fromResources = loadFromResources()

    manifests = fromResources ++ fromExternal
    initialized = true

    System.out.println(s"[VersionRegistry] ${manifests.size} manifeste(s) de version chargé(s): ${manifests.keys.toList.sorted.mkString(", ")}")
  }

  def resolve(mcVersion: McVersion): Option[VManifest] = {
    if (!initialized) init()
    manifests.get(mcVersion.raw).orElse(manifests.get(mcVersion.majorMinor))
  }

  def resolveLogicalMethods(manifest: VManifest, depth: Int = 0): Map[String, LogicalTarget] = {
    if (depth > 16) {
      System.err.println(s"[VersionRegistry] Chaîne 'extends' trop profonde ou cyclique à partir de ${manifest.id}, abandon de la résolution d'héritage.")
      return manifest.logicalMethods
    }

    manifest.extendsId.flatMap(manifests.get) match {
      case Some(parent) =>
        resolveLogicalMethods(parent, depth + 1) ++ manifest.logicalMethods
      case None =>
        manifest.logicalMethods
    }
  }

  def resolveMethod(mcVersion: McVersion, logicalKey: String): Option[LogicalTarget] =
    resolve(mcVersion).flatMap { m =>
      resolveLogicalMethods(m).get(logicalKey)
    }

  def allManifests: Map[String, VManifest] = {
    if (!initialized) init()
    manifests
  }

  private def loadFromDirectory(dir: Path): Map[String, VManifest] = {
    if (!Files.isDirectory(dir)) return Map.empty

    Try {
      Files.list(dir).iterator().asScala
        .filter(p => p.toString.endsWith(".json"))
        .flatMap { p =>
          VersionManifest.load(p) match {
            case Success(m) => Some(m.id -> m)
            case Failure(e) =>
              System.err.println(s"[VersionRegistry] Manifeste invalide ignoré: $p (${e.getMessage})")
              None
          }
        }
        .toMap
    }.getOrElse {
      System.err.println(s"[VersionRegistry] Impossible de lister $dir")
      Map.empty
    }
  }

  private def loadFromResources(): Map[String, VManifest] = {
    val indexStream = getClass.getResourceAsStream("/versions/index.txt")
    if (indexStream == null) return Map.empty

    val names = Try {
      scala.io.Source.fromInputStream(indexStream, "UTF-8").getLines().map(_.trim).filter(_.nonEmpty).toList
    }.getOrElse(Nil)

    names.flatMap { name =>
      val is = getClass.getResourceAsStream(s"/versions/$name")
      if (is == null) {
        System.err.println(s"[VersionRegistry] Manifeste référencé dans l'index introuvable: $name")
        None
      } else {
        val raw = Try(scala.io.Source.fromInputStream(is, "UTF-8").mkString)
        raw.toOption.map(VersionManifest.parse).map(m => m.id -> m)
      }
    }.toMap
  }
}
