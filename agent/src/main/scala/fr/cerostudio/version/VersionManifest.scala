package fr.cerostudio.version

import com.google.gson.{Gson, JsonObject, JsonParser}
import fr.cerostudio.VersionSupport
import fr.cerostudio.VersionSupport.McFlavor

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Path}
import scala.jdk.CollectionConverters._
import scala.util.{Try, Using}

object VersionManifest {

  case class LogicalTarget(className: String, method: String, desc: String) {
    def signature: String = s"$className.$method$desc"
  }

  case class VersionManifest(
    id: String,
    flavor: McFlavor,
    mcJar: String,
    tinyMappings: String,
    mixinConfigs: List[String],
    logicalMethods: Map[String, LogicalTarget],
    extendsId: Option[String] = None
  )

  private val gson = new Gson()

  def load(path: Path): Try[VersionManifest] = Try {
    val raw = new String(Files.readAllBytes(path), StandardCharsets.UTF_8)
    parse(raw)
  }

  def parse(raw: String): VersionManifest = {
    val obj = JsonParser.parseString(raw).getAsJsonObject

    val id   = obj.get("id").getAsString
    val flavorStr = Option(obj.get("flavor")).map(_.getAsString).getOrElse("modern")
    val flavor = flavorStr.toLowerCase match {
      case "legacy" => VersionSupport.Legacy
      case "modern" => VersionSupport.Modern
      case other =>
        System.err.println(s"[VersionManifest] flavor inconnu '$other' pour $id, fallback Modern")
        VersionSupport.Modern
    }

    val mcJar        = obj.get("mcJar").getAsString
    val tinyMappings = obj.get("tinyMappings").getAsString

    val mixinConfigs = Option(obj.getAsJsonArray("mixinConfigs"))
      .map(_.asScala.map(_.getAsString).toList)
      .getOrElse(Nil)

    val extendsId = Option(obj.get("extends")).map(_.getAsString)

    val logicalMethods = Option(obj.getAsJsonObject("logicalMethods"))
      .map(parseLogicalMethods)
      .getOrElse(Map.empty)

    VersionManifest(id, flavor, mcJar, tinyMappings, mixinConfigs, logicalMethods, extendsId)
  }

  private def parseLogicalMethods(obj: JsonObject): Map[String, LogicalTarget] = {
    obj.entrySet().asScala.map { entry =>
      val key = entry.getKey
      val v   = entry.getValue.getAsJsonObject
      key -> LogicalTarget(
        className = v.get("class").getAsString,
        method    = v.get("method").getAsString,
        desc      = v.get("desc").getAsString
      )
    }.toMap
  }
}
