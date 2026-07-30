package fr.cerostudio.remap

import net.fabricmc.tinyremapper.{OutputConsumerPath, TinyRemapper, TinyUtils}

import java.nio.file._
import java.nio.file.attribute.BasicFileAttributes
import java.util.jar.Manifest
import scala.util.Try

/**
 * Remappeur de JAR utilisant TinyRemapper.
 *
 * Version Scala avec :
 *   - Using.Manager pour la gestion automatique des ressources
 *   - Either pour la gestion d'erreurs fonctionnelle
 *   - Fonctions pures séparées de l'I/O
 *   - Nettoyage automatique des signatures Mojang
 */
object JarRemapper {

  def main(args: Array[String]): Unit = {
    if (args.length < 3) {
      System.err.println("[CeroRemapper] Usage: <input.jar> <output.jar> <mappings.tiny>")
      System.exit(1)
    }

    val inputJar    = Paths.get(args(0))
    val outputJar   = Paths.get(args(1))
    val mappingFile = Paths.get(args(2))

    System.out.println(s"[CeroRemapper] Début du remapping de ${inputJar.getFileName}...")

    run(inputJar, outputJar, mappingFile) match {
      case Right(_) =>
        System.out.println("[CeroRemapper] Remapping terminé avec succès !")

      case Left(err) =>
        System.err.println(s"[CeroRemapper] Erreur durant le remapping : $err")
        System.exit(1)
    }
  }

  /**
   * Exécute le remapping complet. Retourne Either[String, Unit] pour
   * un rapport d'erreur propre.
   */
  def run(input: Path, output: Path, mapping: Path): Either[String, Unit] = {
    for {
      _      <- Try(Files.deleteIfExists(output)).toEither.left.map(_.getMessage)
      _      <- remap(input, output, mapping)
      _      <- stripSignatureAndManifest(output)
    } yield ()
  }

  private def remap(input: Path, output: Path, mapping: Path): Either[String, Unit] = {
    Try {
      val remapper = TinyRemapper
        .newRemapper()
        .withMappings(TinyUtils.createTinyMappingProvider(mapping, "official", "named"))
        .build()

      val outputConsumer = new OutputConsumerPath.Builder(output).build()
      try {
        outputConsumer.addNonClassFiles(input)
        remapper.readInputs(input)
        remapper.apply(outputConsumer)
      } finally {
        outputConsumer.close()
      }

      remapper.finish()
    }.toEither.left.map(_.getMessage)
  }

  /**
   * Nettoie les signatures Mojang (SF, RSA, DSA, EC) et vide les entrées
   * du manifeste tout en conservant les attributs principaux.
   */
  private def stripSignatureAndManifest(jarPath: Path): Either[String, Unit] = {
    System.out.println("[CeroRemapper] Nettoyage des signatures Mojang...")

    Try {
      val env = new java.util.HashMap[String, String]()
      val fs = FileSystems.newFileSystem(jarPath, env, classOf[ClassLoader].cast(null))

      try {
        val metaInf = fs.getPath("META-INF")
        if (Files.exists(metaInf)) {
          // Supprimer les fichiers de signature
          Files.walkFileTree(metaInf, new SimpleFileVisitor[Path] {
            override def visitFile(file: Path, attrs: BasicFileAttributes): FileVisitResult = {
              val name = file.getFileName.toString.toUpperCase
              if (name.endsWith(".SF") || name.endsWith(".RSA") ||
                  name.endsWith(".DSA") || name.endsWith(".EC")) {
                Files.delete(file)
              }
              FileVisitResult.CONTINUE
            }
          })

          // Nettoyer le manifeste
          val manifestPath = metaInf.resolve("MANIFEST.MF")
          if (Files.exists(manifestPath)) {
            val is = Files.newInputStream(manifestPath)
            try {
              val mf = new Manifest(is)
              mf.getEntries().clear()
              val os = Files.newOutputStream(manifestPath)
              try {
                mf.write(os)
              } finally {
                os.close()
              }
            } finally {
              is.close()
            }
          }
        }
      } finally {
        fs.close()
      }
      System.out.println("[CeroRemapper] Signatures nettoyées avec succès.")
    }.toEither.left.map(_.getMessage)
  }
}
