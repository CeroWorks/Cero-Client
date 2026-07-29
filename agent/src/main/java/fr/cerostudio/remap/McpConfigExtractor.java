package fr.cerostudio.remap;

import java.io.*;
import java.nio.file.*;
import java.util.zip.*;

public class McpConfigExtractor {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.err.println("Usage: McpConfigExtractor <zip_path> <dest_dir>");
            System.exit(1);
        }

        String zipPath = args[0];
        String destDir = args[1];

        File zipFile = new File(zipPath);
        if (!zipFile.isFile()) {
            System.err.println("[McpConfigExtractor] Fichier zip introuvable: " + zipPath);
            System.exit(1);
        }

        Path destPath = Paths.get(destDir);

        try {
            Files.createDirectories(destPath);
        } catch (IOException e) {
            System.err.println("[McpConfigExtractor] Impossible de créer le dossier destination: " + destDir);
            e.printStackTrace();
            System.exit(1);
        }

        int extractedFiles = 0;

        try (ZipFile zf = new ZipFile(zipFile)) {
            java.util.Enumeration<? extends ZipEntry> entries = zf.entries();

            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();

                Path outPath = destPath.resolve(entry.getName()).normalize();
                if (!outPath.startsWith(destPath)) {
                    System.err.println("[McpConfigExtractor] Entrée suspecte ignorée (zip-slip): " + entry.getName());
                    continue;
                }

                if (entry.isDirectory()) {
                    Files.createDirectories(outPath);
                    continue;
                }

                Path parent = outPath.getParent();
                if (parent != null) {
                    Files.createDirectories(parent);
                }

                try (InputStream is = zf.getInputStream(entry);
                     OutputStream os = Files.newOutputStream(outPath,
                             StandardOpenOption.CREATE,
                             StandardOpenOption.TRUNCATE_EXISTING)) {
                    byte[] buf = new byte[8192];
                    int n;
                    while ((n = is.read(buf)) != -1) {
                        os.write(buf, 0, n);
                    }
                }

                extractedFiles++;
            }
        } catch (IOException e) {
            System.err.println("[McpConfigExtractor] Erreur lors de l'extraction du zip: " + zipPath);
            e.printStackTrace();
            System.exit(1);
        }

        if (extractedFiles == 0) {
            System.err.println("[McpConfigExtractor] Aucun fichier extrait, zip vide ou invalide: " + zipPath);
            System.exit(1);
        }

        System.out.println("[McpConfigExtractor] " + extractedFiles + " fichier(s) extrait(s) vers " + destDir);
        System.exit(0);
    }
}