package fr.cerostudio.remap;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.jar.*;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.FieldNode;

public class LegacySrgToTiny {

    private static class ClassEntry {
        String obfName;
        String namedName;
        Map<String, String> methods = new LinkedHashMap<>();
        Map<String, String> methodDescs = new LinkedHashMap<>();
        Map<String, String> fields = new LinkedHashMap<>();
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.err.println("Usage: LegacySrgToTiny <srg_file> <input_jar> <tiny_out_path>");
            System.exit(1);
        }

        String srgPath = args[0];
        String inputJar = args[1];
        String outPath = args[2];

        try {
            Map<String, ClassEntry> classes = parseSrg(Paths.get(srgPath));
            System.out.println("[LegacySrgToTiny] " + classes.size() + " classes lues depuis " + srgPath);

            Map<String, Map<String, String>> fieldDescs = loadFieldDescriptors(Paths.get(inputJar));
            System.out.println("[LegacySrgToTiny] Descriptors de champs résolus pour " + fieldDescs.size()
                    + " classes depuis " + inputJar);

            writeTiny(Paths.get(outPath), classes, fieldDescs);
            System.out.println("[LegacySrgToTiny] Mapping tiny écrit: " + outPath);
            System.exit(0);

        } catch (Exception e) {
            System.err.println("[LegacySrgToTiny] Erreur pendant la conversion:");
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static Map<String, ClassEntry> parseSrg(Path srgPath) throws IOException {
        Map<String, ClassEntry> classes = new LinkedHashMap<>();
        List<String> lines = Files.readAllLines(srgPath, StandardCharsets.UTF_8);

        for (String raw : lines) {
            String line = raw.trim();
            if (line.isEmpty() || line.startsWith("#") || line.startsWith("PK:")) continue;

            if (line.startsWith("CL:")) {
                String[] parts = line.substring(3).trim().split("\\s+");
                if (parts.length < 2) continue;
                ClassEntry ce = classes.computeIfAbsent(parts[0], k -> new ClassEntry());
                ce.obfName = parts[0];
                ce.namedName = parts[1];

            } else if (line.startsWith("FD:")) {
                String[] parts = line.substring(3).trim().split("\\s+");
                if (parts.length < 2) continue;
                String[] obf = splitOwnerMember(parts[0]);
                String[] named = splitOwnerMember(parts[1]);
                if (obf == null || named == null) continue;
                ClassEntry ce = classes.computeIfAbsent(obf[0], k -> {
                    ClassEntry n = new ClassEntry();
                    n.obfName = obf[0];
                    n.namedName = obf[0];
                    return n;
                });
                ce.fields.put(obf[1], named[1]);

            } else if (line.startsWith("MD:")) {
                String[] parts = line.substring(3).trim().split("\\s+");
                if (parts.length < 4) continue;
                String[] obf = splitOwnerMember(parts[0]);
                String obfDesc = parts[1];
                String[] named = splitOwnerMember(parts[2]);
                if (obf == null || named == null) continue;
                ClassEntry ce = classes.computeIfAbsent(obf[0], k -> {
                    ClassEntry n = new ClassEntry();
                    n.obfName = obf[0];
                    n.namedName = obf[0];
                    return n;
                });
                String key = obf[1] + obfDesc;
                ce.methods.put(key, named[1]);
                ce.methodDescs.put(key, obfDesc);
            }
        }

        return classes;
    }

    
    private static String[] splitOwnerMember(String token) {
        int idx = token.lastIndexOf('/');
        if (idx < 0) return null;
        return new String[]{ token.substring(0, idx), token.substring(idx + 1) };
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
                    System.err.println("[LegacySrgToTiny] Impossible de lire " + entry.getName() + ": " + e.getMessage());
                }
            }
        }

        return result;
    }

    private static void writeTiny(Path outPath, Map<String, ClassEntry> classes,
                                   Map<String, Map<String, String>> fieldDescs) throws IOException {

        Path parent = outPath.getParent();
        if (parent != null) Files.createDirectories(parent);

        try (BufferedWriter w = Files.newBufferedWriter(outPath, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)) {

            w.write("v1\tofficial\tnamed");
            w.newLine();

            for (ClassEntry ce : classes.values()) {
                w.write("CLASS\t" + ce.obfName + "\t" + ce.namedName);
                w.newLine();

                Map<String, String> classFieldDescs = fieldDescs.getOrDefault(ce.obfName, Collections.emptyMap());

                for (Map.Entry<String, String> fieldEntry : ce.fields.entrySet()) {
                    String obfField = fieldEntry.getKey();
                    String namedField = fieldEntry.getValue();
                    String desc = classFieldDescs.get(obfField);

                    if (desc == null) {
                        System.err.println("[LegacySrgToTiny] Skip champ sans desc: " + ce.obfName + "." + obfField);
                        continue;
                    }

                    w.write("FIELD\t" + ce.obfName + "\t" + desc + "\t" + obfField + "\t" + namedField);
                    w.newLine();
                }

                for (Map.Entry<String, String> methodEntry : ce.methods.entrySet()) {
                    String key = methodEntry.getKey();
                    String namedMethod = methodEntry.getValue();
                    String obfDesc = ce.methodDescs.get(key);
                    if (obfDesc == null) continue;

                    String obfMethodName = key.substring(0, key.length() - obfDesc.length());
                    w.write("METHOD\t" + ce.obfName + "\t" + obfDesc + "\t" + obfMethodName + "\t" + namedMethod);
                    w.newLine();
                }
            }
        }
    }
}