package fr.cerostudio

/**
 * Objet utilitaire pour analyser et classifier les versions Minecraft.
 *
 * Couvre toutes les versions connues :
 *   - 1.0 → 1.6  : pré-LWJGL 2 standardisé (support basique)
 *   - 1.7 → 1.12 : « Legacy » — LWJGL 2, Display.setTitle
 *   - 1.13 → 1.16.5 : transition LWJGL 2 → 3
 *   - 1.17 → 1.20.4 : « Modern » — LWJGL 3, Window setTitle / glfwCreateWindow
 *   - 1.21+        : Modern continué
 *   - Snapshots (ex. 25w02a) et versions spéciales (ex. b1.8.1, a1.2.6)
 */
object VersionSupport {

  sealed trait McFlavor
  case object Legacy    extends McFlavor  // 1.7.x – 1.12.x  (LWJGL 2)
  case object Modern   extends McFlavor  // 1.13+            (LWJGL 3)
  case object Unknown  extends McFlavor

  case class McVersion(raw: String, flavor: McFlavor, majorMinor: String) {
    def isLegacy: Boolean  = flavor == Legacy
    def isModern: Boolean  = flavor == Modern
  }

  /**
   * Parse la chaîne de version brute et retourne un McVersion classifié.
   */
  def parse(raw: String): McVersion = {
    val version = raw.trim.toLowerCase

    // Snapshots ex. "25w02a", "24w46a" → traités comme modern
    if (version.matches("""\d{2}w\d{2}[a-z]""")) {
      return McVersion(raw, Modern, version)
    }

    // Versions alpha / bêta historiques ex. "b1.8.1", "a1.2.6"
    if (version.startsWith("b") || version.startsWith("a")) {
      return McVersion(raw, Legacy, version)
    }

    // Extraire le « major.minor » du style 1.X
    val majorMinor = extractMajorMinor(version)

    majorMinor match {
      case Some(mm) =>
        val (major, minor) = mm
        val flavor = classify(major, minor)
        McVersion(raw, flavor, s"$major.$minor")

      case None =>
        // Format non reconnu — tenter de matcher un pattern 1.X.Y
        if (version.startsWith("1.")) {
          // Fallback: traiter comme modern par défaut si >= 1.13
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

  /**
   * Retourne le nom de fichier de config mixin approprié pour cette version.
   */
  def mixinConfig(mcVersion: McVersion): String = mcVersion.flavor match {
    case Legacy  => "mixins.cero.v1_legacy.json"
    case Modern  => "mixins.cero.v1_modern.json"
    case Unknown =>
      System.err.println(s"[VersionSupport] Version non reconnue '${mcVersion.raw}', utilisation du config modern par défaut.")
      "mixins.cero.v1_modern.json"
  }

  // ── Internes ──────────────────────────────────────────────────────

  /** Classification par numéros de version */
  private def classify(major: Int, minor: Int): McFlavor = {
    if (major == 1) {
      if (minor <= 12) Legacy
      else Modern      // 1.13+
    } else {
      // Si un jour Minecraft 2.0 existe… on traitera comme modern
      System.err.println(s"[VersionSupport] Version majeure inattendue: $major.$minor")
      Modern
    }
  }

  /**
   * Extrait (major, minor) depuis une chaîne comme "1.12.2" → Some((1, 12))
   */
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
