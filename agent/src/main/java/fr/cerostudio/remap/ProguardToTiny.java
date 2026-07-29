package fr.cerostudio.remap;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class ProguardToTiny {

    private static final Map<String, String> PRIMITIVES = new HashMap<>();
    static {
        PRIMITIVES.put("void", "V");
        PRIMITIVES.put("boolean", "Z");
        PRIMITIVES.put("byte", "B");
        PRIMITIVES.put("char", "C");
        PRIMITIVES.put("short", "S");
        PRIMITIVES.put("int", "I");
        PRIMITIVES.put("long", "J");
        PRIMITIVES.put("float", "F");
        PRIMITIVES.put("double", "D");
    }

    private static final Pattern CLASS_LINE =
        Pattern.compile("^([\\w.$]+)\\s*->\\s*([\\w.$]+):\\s*$");

    private static final Pattern FIELD_LINE =
        Pattern.compile("^\\s+([\\w.$\\[\\]]+)\\s+([\\w$]+)\\s*->\\s*([\\w$]+)\\s*$");

    private static final Pattern METHOD_LINE =
        Pattern.compile("^\\s+(?:\\d+:\\d+:)?([\\w.$\\[\\]]+)\\s+([\\w$<>]+)\\(([^)]*)\\)\\s*->\\s*([\\w$]+)\\s*$");

    public static void main(String[] args) throws IOException {
        if (args.length < 2) {
            System.err.println("Usage: ProguardToTiny <input.proguard.txt> <output.tiny>");
            System.exit(1);
        }

        Path input = Paths.get(args[0]);
        Path output = Paths.get(args[1]);

        List<String> lines = Files.readAllLines(input, java.nio.charset.StandardCharsets.UTF_8);

        Map<String, String> classDeobfToObf = new HashMap<>();
        for (String line : lines) {
            if (line.startsWith("#")) continue;
            Matcher m = CLASS_LINE.matcher(line);
            if (m.matches()) {
                classDeobfToObf.put(m.group(1), m.group(2));
            }
        }

        StringBuilder sb = new StringBuilder();
        sb.append("v1\tofficial\tnamed\n");

        String currentObfClass = null;
        String currentDeobfClass = null;

        for (String line : lines) {
            if (line.startsWith("#") || line.trim().isEmpty()) continue;

            Matcher cm = CLASS_LINE.matcher(line);
            if (cm.matches()) {
                currentDeobfClass = cm.group(1);
                currentObfClass = cm.group(2);
                sb.append("CLASS\t")
                  .append(currentObfClass.replace('.', '/')).append('\t')
                  .append(currentDeobfClass.replace('.', '/')).append('\n');
                continue;
            }

            if (currentObfClass == null) continue;

            Matcher fm = FIELD_LINE.matcher(line);
            if (fm.matches() && !line.contains("(")) {
                String type = fm.group(1);
                String deobfName = fm.group(2);
                String obfName = fm.group(3);
                String desc = toDescriptor(type, classDeobfToObf);
                sb.append("FIELD\t")
                  .append(currentObfClass.replace('.', '/')).append('\t')
                  .append(desc).append('\t')
                  .append(obfName).append('\t')
                  .append(deobfName).append('\n');
                continue;
            }

            Matcher mm = METHOD_LINE.matcher(line);
            if (mm.matches()) {
                String retType = mm.group(1);
                String deobfName = mm.group(2);
                String paramsRaw = mm.group(3);
                String obfName = mm.group(4);

                String desc = buildMethodDescriptor(retType, paramsRaw, classDeobfToObf);
                sb.append("METHOD\t")
                  .append(currentObfClass.replace('.', '/')).append('\t')
                  .append(desc).append('\t')
                  .append(obfName).append('\t')
                  .append(deobfName).append('\n');
            }
        }

        Files.write(output, sb.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
        System.out.println("[ProguardToTiny] OK -> " + output);
    }

    private static String buildMethodDescriptor(String retType, String paramsRaw,
                                                  Map<String, String> classMap) {
        StringBuilder params = new StringBuilder("(");
        if (!paramsRaw.trim().isEmpty()) {
            for (String p : paramsRaw.split(",")) {
                params.append(toDescriptor(p.trim(), classMap));
            }
        }
        params.append(")").append(toDescriptor(retType, classMap));
        return params.toString();
    }

    private static String toDescriptor(String type, Map<String, String> classMap) {
        int arrayDepth = 0;
        while (type.endsWith("[]")) {
            arrayDepth++;
            type = type.substring(0, type.length() - 2);
        }

        String desc;
        if (PRIMITIVES.containsKey(type)) {
            desc = PRIMITIVES.get(type);
        } else {
            String obf = classMap.getOrDefault(type, type);
            desc = "L" + obf.replace('.', '/') + ";";
        }

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < arrayDepth; i++) sb.append('[');
        sb.append(desc);
        return sb.toString();
    }
}