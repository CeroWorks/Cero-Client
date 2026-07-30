package fr.cerostudio.remap

import java.io.{BufferedWriter, IOException, InputStream}
import java.nio.charset.StandardCharsets
import java.nio.file._
import java.nio.file.StandardOpenOption._
import java.util.jar.{JarEntry, JarFile}
import java.util.{Enumeration => JEnumeration}
import org.objectweb.asm.{ClassReader => AsmClassReader}
import org.objectweb.asm.tree.{ClassNode, FieldNode}
import scala.collection.mutable
import scala.jdk.CollectionConverters._
import scala.util.{Try, Using, Failure, Success}

/**
 * Convertisseur MCP (TSRG + CSV) → Tiny v2.
 *
 * Version Scala utilisant :
 *   - Case classes pour la modélisation des données
 *   - Pattern matching sur les lignes du TSRG
 *   - Collections mutables locales pour la performance
 *   - Try/Either pour la gestion d'erreurs explicite
 */
object McpToTiny {

  /** Entrée de classe avec mappings de méthodes et champs */
  case class ClassEntry(
    obfName: String,
    srgName: String,
    methods: mutable.LinkedHashMap[String, String] = mutable.LinkedHashMap.empty,
    fields: mutable.LinkedHashMap[String, String] = mutable.LinkedHashMap.empty,
    methodDescs: mutable.LinkedHashMap[String, String] = mutable.LinkedHashMap.empty
  )

  def main(args: Array[String]): Unit = {
    if (args.length < 3) {
      System.err.println("Usage: McpToTiny <mcp_dir> <input_jar> <tiny_out_path>")
      System.exit(1)
    }

    val mcpDir   = args(0)
    val inputJar = args(1)
    val outPath  = args(2)

    run(mcpDir, inputJar, outPath) match {
      case Success(_) =>
        System.out.println(s"[McpToTiny] Mapping tiny écrit: $outPath")
        System.exit(0)

      case Failure(e) =>
        System.err.println("[McpToTiny] Erreur pendant la conversion:")
        e.printStackTrace()
        System.exit(1)
    }
  }

  def run(mcpDir: String, inputJar: String, outPath: String): Try[Unit] = Try {
    val tsrgPath = findTsrg(Paths.get(mcpDir)).getOrElse {
      System.err.println(s"[McpToTiny] Aucun fichier TSRG trouvé dans $mcpDir")
      System.exit(1)
      throw new IllegalStateException("TSRG not found")
    }
    System.out.println(s"[McpToTiny] TSRG utilisé: $tsrgPath")

    val classes = parseTsrg(tsrgPath)
    System.out.println(s"[McpToTiny] ${classes.size} classes lues depuis le TSRG")

    val methodNames = loadCsvMapping(Paths.get(mcpDir, "config", "methods.csv"))
    val fieldNames  = loadCsvMapping(Paths.get(mcpDir, "config", "fields.csv"))
    System.out.println(
      s"[McpToTiny] ${methodNames.size} noms de méthodes, ${fieldNames.size} noms de champs chargés depuis les CSV"
    )

    val fieldDescs = loadFieldDescriptors(Paths.get(inputJar))
    System.out.println(s"[McpToTiny] Descriptors de champs résolus pour ${fieldDescs.size} classes depuis $inputJar")

    writeTiny(Paths.get(outPath), classes, methodNames, fieldNames, fieldDescs)
  }

  // ── Chargement des field descriptors depuis le JAR ─────────────────

  def loadFieldDescriptors(jarPath: Path): Map[String, Map[String, String]] = {
    val result = mutable.Map[String, Map[String, String]]()

    Using(new JarFile(jarPath.toFile)) { jf =>
      val entries: JEnumeration[JarEntry] = jf.entries()
      while (entries.hasMoreElements) {
        val entry = entries.nextElement()
        if (!entry.isDirectory && entry.getName.endsWith(".class")) {
          Using.resource(jf.getInputStream(entry)) { is =>
            Try {
              val cr = new AsmClassReader(is)
              val cn = new ClassNode()
              cr.accept(cn, AsmClassReader.SKIP_CODE | AsmClassReader.SKIP_DEBUG | AsmClassReader.SKIP_FRAMES)
              val fields = cn.fields.asScala.map(fn => fn.name -> fn.desc).toMap
              result(cn.name) = fields
            }.recover {
              case e =>
                System.err.println(s"[McpToTiny] Impossible de lire ${entry.getName}: ${e.getMessage}")
            }
          }
        }
      }
    }.fold(
      { e =>
        System.err.println(s"[McpToTiny] Erreur lecture JAR: ${e.getMessage}")
        Map.empty[String, Map[String, String]]
      },
      _ => result.toMap
    )
  }

  // ── Recherche du fichier TSRG ──────────────────────────────────────

  def findTsrg(mcpDir: Path): Option[Path] = {
    val candidates = Seq("joined.tsrg", "client.tsrg", "server.tsrg")
    val configDir  = mcpDir.resolve("config")

    // Chercher en priorité dans config/
    candidates.iterator.map(configDir.resolve).find(Files.isRegularFile(_)) match {
      case Some(p) => Some(p)
      case None =>
        // Fallback: scan récursif via Files.walk
        val stream = Files.walk(mcpDir)
        try {
          val it = stream.iterator()
          var found: Option[Path] = None
          while (it.hasNext && found.isEmpty) {
            val p = it.next()
            if (p.toString.endsWith(".tsrg")) found = Some(p)
          }
          found
        } finally {
          stream.close()
        }
    }
  }

  // ── Parsing TSRG ───────────────────────────────────────────────────

  def parseTsrg(tsrgPath: Path): Map[String, ClassEntry] = {
    val source = scala.io.Source.fromFile(tsrgPath.toFile, StandardCharsets.UTF_8.name())
    try {
      val classes  = mutable.LinkedHashMap[String, ClassEntry]()
      var current: Option[ClassEntry] = None

      for (rawLine <- source.getLines()) {
        val line = rawLine.trim
        if (line.nonEmpty && !line.startsWith("#")) {
          val indented = rawLine.startsWith("\t") || rawLine.startsWith("    ")
          val parts    = line.split("\\s+")

          if (!indented && parts.length >= 2) {
            val entry = ClassEntry(obfName = parts(0), srgName = parts(1))
            classes(entry.obfName) = entry
            current = Some(entry)
          } else if (indented && current.isDefined) {
            val ce = current.get
            parts.length match {
              case 3 =>
                // Méthode: obfMethod obfDesc srgMethod
                val key = parts(0) + parts(1)
                ce.methods(key) = parts(2)
                ce.methodDescs(key) = parts(1)

              case 2 =>
                // Champ: obfField srgField
                ce.fields(parts(0)) = parts(1)

              case _ => // Ignorer
            }
          }
        }
      }

      classes.toMap
    } finally {
      source.close()
    }
  }

  // ── Chargement CSV ──────────────────────────────────────────────────

  def loadCsvMapping(csvPath: Path): Map[String, String] = {
    if (!Files.isRegularFile(csvPath)) return Map.empty

    val source = scala.io.Source.fromFile(csvPath.toFile, StandardCharsets.UTF_8.name())
    try {
      val lines   = source.getLines().toList
      if (lines.isEmpty) return Map.empty

      val header  = lines.head.split(",", -1)
      val seargeIdx = indexOf(header, "searge")
      val nameIdx   = indexOf(header, "name")

      if (seargeIdx < 0 || nameIdx < 0) {
        System.err.println(s"[McpToTiny] En-tête CSV inattendu dans $csvPath: ${lines.head}")
        return Map.empty
      }

      lines.tail.iterator.flatMap { line =>
        if (line.isEmpty) None
        else {
          val cols = if (line.contains("\t")) line.split("\t", -1) else line.split(",", -1)
          if (cols.length <= math.max(seargeIdx, nameIdx)) None
          else {
            val srge = cols(seargeIdx).trim
            val name = cols(nameIdx).trim
            if (srge.nonEmpty && name.nonEmpty) Some(srge -> name) else None
          }
        }
      }.toMap
    } finally {
      source.close()
    }
  }

  private def indexOf(arr: Array[String], target: String): Int =
    arr.indexWhere(_.trim.equalsIgnoreCase(target))

  // ── Écriture Tiny v1 ────────────────────────────────────────────────

  def writeTiny(
    outPath: Path,
    classes: Map[String, ClassEntry],
    methodNames: Map[String, String],
    fieldNames: Map[String, String],
    fieldDescs: Map[String, Map[String, String]]
  ): Unit = {
    Option(outPath.getParent).foreach(p => Files.createDirectories(p))

    Using.resource(
      Files.newBufferedWriter(outPath, StandardCharsets.UTF_8, CREATE, TRUNCATE_EXISTING)
    ) { w =>
      w.write("v1\tofficial\tnamed")
      w.newLine()

      for (ce <- classes.values) {
        w.write(s"CLASS\t${ce.obfName}\t${ce.srgName}")
        w.newLine()

        val classFieldDescs = fieldDescs.getOrElse(ce.obfName, Map.empty)

        // Champs
        for ((obfField, srgField) <- ce.fields) {
          val named = fieldNames.getOrElse(srgField, srgField)
          classFieldDescs.get(obfField) match {
            case None =>
              System.err.println(s"[McpToTiny] Skip champ sans desc: ${ce.obfName}.$obfField")
            case Some(desc) =>
              w.write(s"FIELD\t${ce.obfName}\t$desc\t$obfField\t$named")
              w.newLine()
          }
        }

        // Méthodes
        for ((key, srgMethod) <- ce.methods) {
          val obfDesc = ce.methodDescs.getOrElse(key, "")
          if (obfDesc.isEmpty) {
            System.err.println(s"[McpToTiny] Skip méthode sans desc: ${ce.obfName}.$key")
          } else {
            val obfMethodName = key.substring(0, key.length - obfDesc.length)
            val named = methodNames.getOrElse(srgMethod, srgMethod)
            w.write(s"METHOD\t${ce.obfName}\t$obfDesc\t$obfMethodName\t$named")
            w.newLine()
          }
        }
      }
    }
  }
}
