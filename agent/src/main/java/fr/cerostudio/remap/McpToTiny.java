package fr.cerostudio.remap;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.jar.*;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.FieldNode;

public class McpToTiny {

    private static class ClassEntry {
        String obfName;
        String srgName;
        Map<String, String> methods = new LinkedHashMap<>();
        Map<String, String> fields = new LinkedHashMap<>();
        Map<String, String> methodDescs = new LinkedHashMap<>();
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.err.println("Usage: McpToTiny <mcp_dir> <input_jar> <tiny_out_path>");
            System.exit(1);
        }

        String mcpDir = args[0];
        String inputJar = args[1];
        String outPath = args[2];

        try {
            Path tsrgPath = findTsrg(Paths.get(mcpDir));
            if (tsrgPath == null) {
                System.err.println("[McpToTiny] Aucun fichier TSRG trouvé dans " + mcpDir);
                System.exit(1);
                return;
            }
            System.out.println("[McpToTiny] TSRG utilisé: " + tsrgPath);

            Map<String, ClassEntry> classes = parseTsrg(tsrgPath);
            System.out.println("[McpToTiny] " + classes.size() + " classes lues depuis le TSRG");

            Map<String, String> methodNames = loadCsvMapping(Paths.get(mcpDir, "config", "methods.csv"));
            Map<String, String> fieldNames = loadCsvMapping(Paths.get(mcpDir, "config", "fields.csv"));
            System.out.println("[McpToTiny] " + methodNames.size() + " noms de méthodes, "
                    + fieldNames.size() + " noms de champs chargés depuis les CSV");

            Map<String, Map<String, String>> fieldDescs = loadFieldDescriptors(Paths.get(inputJar));
            System.out.println("[McpToTiny] Descriptors de champs résolus pour " + fieldDescs.size() + " classes depuis " + inputJar);

            writeTiny(Paths.get(outPath), classes, methodNames, fieldNames, fieldDescs);
            System.out.println("[McpToTiny] Mapping tiny écrit: " + outPath);
            System.exit(0);

        } catch (Exception e) {
            System.err.println("[McpToTiny] Erreur pendant la conversion:");
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static Map<String, Map<String, String>> loadFieldDescriptors(Path jarPath) throws IOException {
        Map<String, Map<String, String>> result = new HashMap<>();

        try (JarFile jf = new JarFile(jarPath.toFile())) {
            Enumeration<JarEntry> entries = jf.entries();
            while (entries.hasMoreElements()) {
                JarEntry entry = entries.nextElement();
                if (entry.isDirectory() || !entry.getName().endsWith(".class")) continue;

                try (InputStream is = jf.getInputStream(entry)) {
                    ClassReader cr = new ClassReader(is);
                    ClassNode cn = new ClassNode();
                    cr.accept(cn, ClassReader.SKIP_CODE | ClassReader.SKIP_DEBUG | ClassReader.SKIP_FRAMES);

                    Map<String, String> fields = new HashMap<>();
                    for (FieldNode fn : cn.fields) {
                        fields.put(fn.name, fn.desc);
                    }
                    result.put(cn.name, fields);
                } catch (Exception e) {
                    System.err.println("[McpToTiny] Impossible de lire " + entry.getName() + ": " + e.getMessage());
                }
            }
        }

        return result;
    }

    private static Path findTsrg(Path mcpDir) throws IOException {
        String[] candidates = { "joined.tsrg", "client.tsrg", "server.tsrg" };
        Path configDir = mcpDir.resolve("config");
        for (String name : candidates) {
            Path p = configDir.resolve(name);
            if (Files.isRegularFile(p)) return p;
        }
        final Path[] found = { null };
        try (java.util.stream.Stream<Path> walk = Files.walk(mcpDir)) {
            walk.forEach(p -> {
                if (found[0] == null && p.toString().endsWith(".tsrg")) {
                    found[0] = p;
                }
            });
        }
        return found[0];
    }

    private static Map<String, ClassEntry> parseTsrg(Path tsrgPath) throws IOException {
        Map<String, ClassEntry> classes = new LinkedHashMap<>();
        List<String> lines = Files.readAllLines(tsrgPath, StandardCharsets.UTF_8);

        ClassEntry current = null;

        for (String rawLine : lines) {
            if (rawLine.isEmpty()) continue;
            if (rawLine.startsWith("#")) continue;

            boolean indented = rawLine.startsWith("\t") || rawLine.startsWith("    ");
            String line = rawLine.trim();
            if (line.isEmpty()) continue;

            String[] parts = line.split("\\s+");

            if (!indented) {
                if (parts.length < 2) continue;
                ClassEntry ce = new ClassEntry();
                ce.obfName = parts[0];
                ce.srgName = parts[1];
                classes.put(ce.obfName, ce);
                current = ce;
            } else {
                if (current == null) continue;
                if (parts.length == 3) {
                    String obfMethod = parts[0];
                    String obfDesc = parts[1];
                    String srgMethod = parts[2];
                    String key = obfMethod + obfDesc;
                    current.methods.put(key, srgMethod);
                    current.methodDescs.put(key, obfDesc);
                } else if (parts.length == 2) {
                    String obfField = parts[0];
                    String srgField = parts[1];
                    current.fields.put(obfField, srgField);
                }
            }
        }

        return classes;
    }

    private static Map<String, String> loadCsvMapping(Path csvPath) throws IOException {
        Map<String, String> result = new HashMap<>();
        if (!Files.isRegularFile(csvPath)) {
            return result;
        }

        List<String> lines = Files.readAllLines(csvPath, StandardCharsets.UTF_8);
        if (lines.isEmpty()) return result;

        String header = lines.get(0);
        String[] headerCols = header.split(",", -1);
        int seargeIdx = indexOf(headerCols, "searge");
        int nameIdx = indexOf(headerCols, "name");

        if (seargeIdx < 0 || nameIdx < 0) {
            System.err.println("[McpToTiny] En-tête CSV inattendu dans " + csvPath + ": " + header);
            return result;
        }

        for (int i = 1; i < lines.size(); i++) {
            String line = lines.get(i);
            if (line.isEmpty()) continue;
            String[] cols = splitCsvLine(line);
            if (cols.length <= Math.max(seargeIdx, nameIdx)) continue;

            String srge = cols[seargeIdx].trim();
            String name = cols[nameIdx].trim();
            if (!srge.isEmpty() && !name.isEmpty()) {
                result.put(srge, name);
            }
        }

        return result;
    }

    private static int indexOf(String[] arr, String target) {
        for (int i = 0; i < arr.length; i++) {
            if (arr[i].trim().equalsIgnoreCase(target)) return i;
        }
        return -1;
    }

    private static String[] splitCsvLine(String line) {
        if (line.contains("\t")) {
            return line.split("\t", -1);
        }
        return line.split(",", -1);
    }

    private static void writeTiny(Path outPath, Map<String, ClassEntry> classes,
                               Map<String, String> methodNames,
                               Map<String, String> fieldNames,
                               Map<String, Map<String, String>> fieldDescs) throws IOException {

    Path parent = outPath.getParent();
    if (parent != null) Files.createDirectories(parent);

    try (BufferedWriter w = Files.newBufferedWriter(outPath, StandardCharsets.UTF_8,
            StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)) {

        w.write("v1\tofficial\tnamed");
        w.newLine();

        for (ClassEntry ce : classes.values()) {
            w.write("CLASS\t" + ce.obfName + "\t" + ce.srgName);
            w.newLine();

            Map<String, String> classFieldDescs = fieldDescs.getOrDefault(ce.obfName, Collections.emptyMap());

            for (Map.Entry<String, String> fieldEntry : ce.fields.entrySet()) {
                String obfField = fieldEntry.getKey();
                String srgField = fieldEntry.getValue();
                String named = fieldNames.getOrDefault(srgField, srgField);
                String desc = classFieldDescs.getOrDefault(obfField, "");
                
                if (desc.isEmpty()) {
                    System.err.println("[McpToTiny] Skip champ sans desc: " + ce.obfName + "." + obfField);
                    continue;
                }
                
                w.write("FIELD\t" + ce.obfName + "\t" + desc + "\t" + obfField + "\t" + named);
                w.newLine();
            }

            for (Map.Entry<String, String> methodEntry : ce.methods.entrySet()) {
                String key = methodEntry.getKey();
                String srgMethod = methodEntry.getValue();
                String obfDesc = ce.methodDescs.get(key);
                
                if (obfDesc == null || obfDesc.isEmpty()) {
                    System.err.println("[McpToTiny] Skip méthode sans desc: " + ce.obfName + "." + key);
                    continue;
                }
                
                String obfMethodName = key.substring(0, key.length() - obfDesc.length());
                String named = methodNames.getOrDefault(srgMethod, srgMethod);
                w.write("METHOD\t" + ce.obfName + "\t" + obfDesc + "\t" + obfMethodName + "\t" + named);
                w.newLine();
            }
        }
    }
}
}