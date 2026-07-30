package fr.cerostudio.remap

import java.io.{File, IOException, InputStream, OutputStream}
import java.nio.file._
import java.nio.file.attribute.BasicFileAttributes
import java.util.zip.{ZipEntry, ZipFile}
import scala.util.{Using, Try, Success, Failure}

/**
 * Extracteur de configurations MCP depuis un zip.
 *
 * Version Scala avec :
 *   - Protection anti zip-slip
 *   - Utilisation de Using pour la gestion automatique des ressources
 *   - Logs détaillés en français
 */
object McpConfigExtractor {

  def main(args: Array[String]): Unit = {
    if (args.length < 2) {
      System.err.println("Usage: McpConfigExtractor <zip_path> <dest_dir>")
      System.exit(1)
    }

    val zipPath = args(0)
    val destDir = args(1)

    val result = extractZip(zipPath, destDir)

    result match {
      case Success(count) =>
        System.out.println(s"[McpConfigExtractor] $count fichier(s) extrait(s) vers $destDir")
        System.exit(0)

      case Failure(e) =>
        System.err.println(s"[McpConfigExtractor] Erreur: ${e.getMessage}")
        e.printStackTrace()
        System.exit(1)
    }
  }

  /**
   * Extrait un zip avec protection anti zip-slip et retourne le nombre
   * de fichiers extraits.
   */
  def extractZip(zipPath: String, destDir: String): Try[Int] = {
    val zipFile = new File(zipPath)
    if (!zipFile.isFile) {
      return Failure(new IOException(s"Fichier zip introuvable: $zipPath"))
    }

    val destPath = Paths.get(destDir)
    Files.createDirectories(destPath)

    Using(new ZipFile(zipFile)) { zf =>
      var extracted = 0
      val entries   = zf.entries()

      while (entries.hasMoreElements) {
        val entry = entries.nextElement()
        val outPath = destPath.resolve(entry.getName).normalize

        // Protection anti zip-slip
        if (!outPath.startsWith(destPath)) {
          System.err.println(s"[McpConfigExtractor] Entrée suspecte ignorée (zip-slip): ${entry.getName}")
        } else if (entry.isDirectory) {
          Files.createDirectories(outPath)
        } else {
          Files.createDirectories(outPath.getParent)
          copyStream(zf.getInputStream(entry), outPath)
          extracted += 1
        }
      }

      if (extracted == 0) {
        Failure(new IOException(s"Aucun fichier extrait, zip vide ou invalide: $zipPath"))
      } else {
        Success(extracted)
      }
    }.flatten
  }

  private def copyStream(is: InputStream, target: Path): Unit = {
    Using.resource(is) { in =>
      Using.resource(Files.newOutputStream(target, StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)) { out =>
        val buf = new Array[Byte](8192)
        var n = in.read(buf)
        while (n != -1) {
          out.write(buf, 0, n)
          n = in.read(buf)
        }
      }
    }
  }
}
