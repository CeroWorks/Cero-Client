package fr.cerostudio

object VersionSupport {

  sealed trait McFlavor
  case object Legacy    extends McFlavor
  case object Modern   extends McFlavor
  case object Unknown  extends McFlavor

  case class McVersion(raw: String, flavor: McFlavor, majorMinor: String) {
    def isLegacy: Boolean  = flavor == Legacy
    def isModern: Boolean  = flavor == Modern
  }

  def parse(raw: String): McVersion = {
    val version = raw.trim.toLowerCase

    if (version.matches("""\d{2}w\d{2}[a-z]""")) {
      return McVersion(raw, Modern, version)
    }

    if (version.startsWith("b") || version.startsWith("a")) {
      return McVersion(raw, Legacy, version)
    }

    val majorMinor = extractMajorMinor(version)

    majorMinor match {
      case Some(mm) =>
        val (major, minor) = mm
        val flavor = classify(major, minor)
        McVersion(raw, flavor, s"$major.$minor")

      case None =>
        if (version.startsWith("1.")) {
          val rest = version.drop(2)
          val minorOpt = rest.takeWhile(_.isDigit)
          if (minorOpt.nonEmpty) {
            val minor = minorOpt.toInt
            val flavor = if (minor >= 13) Modern else Legacy
            McVersion(raw, flavor, s"1.$minor")
          } else {
            McVersion(raw, Unknown, version)
          }
        } else {
          McVersion(raw, Unknown, version)
        }
    }
  }

  def mixinConfigs(mcVersion: McVersion): List[String] = {
    fr.cerostudio.version.VersionRegistry.resolve(mcVersion) match {
      case Some(manifest) if manifest.mixinConfigs.nonEmpty =>
        manifest.mixinConfigs
      case _ =>
        List(mixinConfig(mcVersion))
    }
  }

  def mixinConfig(mcVersion: McVersion): String = mcVersion.flavor match {
    case Legacy  => "mixins.cero.v1_legacy.json"
    case Modern  => "mixins.cero.v1_modern.json"
    case Unknown =>
      System.err.println(s"[VersionSupport] Version non reconnue '${mcVersion.raw}', utilisation du config modern par défaut.")
      "mixins.cero.v1_modern.json"
  }

  private def classify(major: Int, minor: Int): McFlavor = {
    if (major == 1) {
      if (minor <= 12) Legacy
      else Modern
    } else {
      System.err.println(s"[VersionSupport] Version majeure inattendue: $major.$minor")
      Modern
    }
  }

  private def extractMajorMinor(version: String): Option[(Int, Int)] = {
    val parts = version.split('.')
    if (parts.length >= 2) {
      val major = parts(0).filter(_.isDigit)
      val minor = parts(1).filter(_.isDigit)
      if (major.nonEmpty && minor.nonEmpty) {
        Some((major.toInt, minor.toInt))
      } else None
    } else None
  }
}
