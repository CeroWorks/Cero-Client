package fr.cerostudio.remap

import java.nio.charset.StandardCharsets
import java.nio.file._
import scala.jdk.CollectionConverters._
import scala.collection.mutable
import scala.util.matching.Regex

/**
 * Convertisseur Proguard → Tiny v1.
 *
 * Version Scala utilisant :
 *   - Regex compilées comme vals
 *   - Map immutable pour les primitives
 *   - Pattern matching sur les matchers
 *   - StringBuilder avec interpolation
 */
object ProguardToTiny {

  /** Mapping type Java → descripteur JVM */
  private val Primitives: Map[String, String] = Map(
    "void"    -> "V",
    "boolean" -> "Z",
    "byte"    -> "B",
    "char"    -> "C",
    "short"   -> "S",
    "int"     -> "I",
    "long"    -> "J",
    "float"   -> "F",
    "double"  -> "D"
  )

  // ── Regex de parsing Proguard ───────────────────────────────────────

  private val ClassLineRegex: Regex =
    """^([\w.$]+)\s*->\s*([\w.$]+):\s*$""".r

  private val FieldLineRegex: Regex =
    """^\s+([\w.$\[\]]+)\s+([\w$]+)\s*->\s*([\w$]+)\s*$""".r

  private val MethodLineRegex: Regex =
    """^\s+(?:\d+:\d+:)?([\w.$\[\]]+)\s+([\w$<>]+)\(([^)]*)\)\s*->\s*([\w$]+)\s*$""".r

  def main(args: Array[String]): Unit = {
    if (args.length < 2) {
      System.err.println("Usage: ProguardToTiny <input.proguard.txt> <output.tiny>")
      System.exit(1)
    }

    val input  = Paths.get(args(0))
    val output = Paths.get(args(1))

    try {
      convert(input, output)
      System.out.println(s"[ProguardToTiny] OK -> $output")
    } catch {
      case e: Exception =>
        System.err.println(s"[ProguardToTiny] Erreur: ${e.getMessage}")
        e.printStackTrace()
        System.exit(1)
    }
  }

  def convert(input: Path, output: Path): Unit = {
    val lines = Files.readAllLines(input, StandardCharsets.UTF_8).asScala.toList

    // 1ère passe : construire la table classDeobfToObf
    val classDeobfToObf = mutable.HashMap[String, String]()
    for (line <- lines; if !line.startsWith("#")) {
      line match {
        case ClassLineRegex(deobf, obf) => classDeobfToObf(deobf) = obf
        case _ => // Ignorer
      }
    }

    // 2ème passe : générer le Tiny
    val sb = new StringBuilder()
    sb.append("v1\tofficial\tnamed\n")

    var currentObfClass: String = null
    var currentDeobfClass: String = null

    for (line <- lines; if !line.startsWith("#") && line.trim.nonEmpty) {
      line match {
        case ClassLineRegex(deobf, obf) =>
          currentDeobfClass = deobf
          currentObfClass = obf
          sb.append("CLASS\t")
            .append(obf.replace('.', '/')).append('\t')
            .append(deobf.replace('.', '/')).append('\n')

        case FieldLineRegex(fieldType, deobfName, obfName) if currentObfClass != null && !line.contains("(") =>
          val desc = toDescriptor(fieldType, classDeobfToObf.toMap)
          sb.append("FIELD\t")
            .append(currentObfClass.replace('.', '/')).append('\t')
            .append(desc).append('\t')
            .append(obfName).append('\t')
            .append(deobfName).append('\n')

        case MethodLineRegex(retType, deobfName, paramsRaw, obfName) if currentObfClass != null =>
          val desc = buildMethodDescriptor(retType, paramsRaw, classDeobfToObf.toMap)
          sb.append("METHOD\t")
            .append(currentObfClass.replace('.', '/')).append('\t')
            .append(desc).append('\t')
            .append(obfName).append('\t')
            .append(deobfName).append('\n')

        case _ => // Ignorer les lignes non-matchées
      }
    }

    Files.write(output, sb.toString.getBytes(StandardCharsets.UTF_8))
  }

  // ── Construction des descripteurs ───────────────────────────────────

  private def buildMethodDescriptor(
    retType: String,
    paramsRaw: String,
    classMap: Map[String, String]
  ): String = {
    val params = new StringBuilder("(")
    if (paramsRaw.trim.nonEmpty) {
      for (p <- paramsRaw.split(",")) {
        params.append(toDescriptor(p.trim, classMap))
      }
    }
    params.append(")").append(toDescriptor(retType, classMap))
    params.toString
  }

  private def toDescriptor(typeStr: String, classMap: Map[String, String]): String = {
    // Gérer les tableaux (ex. "int[]", "String[][]")
    var t = typeStr
    var arrayDepth = 0
    while (t.endsWith("[]")) {
      arrayDepth += 1
      t = t.substring(0, t.length - 2)
    }

    val desc = Primitives.getOrElse(
      t,
      "L" + classMap.getOrElse(t, t).replace('.', '/') + ";"
    )

    "[" * arrayDepth + desc
  }
}
