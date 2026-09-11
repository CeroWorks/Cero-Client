package fr.cerostudio.api.capability;

import java.util.Collections;
import java.util.EnumSet;
import java.util.Set;

public final class CapabilitySet {

    private final String mcVersion;
    private final Set<GameCapability> capabilities;

    private CapabilitySet(String mcVersion, Set<GameCapability> capabilities) {
        this.mcVersion = mcVersion;
        this.capabilities = Collections.unmodifiableSet(capabilities);
    }

    public boolean has(GameCapability capability) {
        return capabilities.contains(capability);
    }

    public String mcVersion() {
        return mcVersion;
    }

    public Set<GameCapability> all() {
        return capabilities;
    }

    public static CapabilitySet resolve(String mcVersion) {
        EnumSet<GameCapability> caps = EnumSet.noneOf(GameCapability.class);

        String majorMinor = majorMinor(mcVersion);

        if (isLegacy(majorMinor)) {
            caps.add(GameCapability.LWJGL2_DISPLAY);
            caps.add(GameCapability.MCP_MAPPINGS);
        } else {
            caps.add(GameCapability.LWJGL3_GLFW);
            caps.add(GameCapability.PROGUARD_MAPPINGS);
        }

        return new CapabilitySet(mcVersion, caps);
    }

    private static boolean isLegacy(String majorMinor) {
        switch (majorMinor) {
            case "1.7":
            case "1.8":
            case "1.9":
            case "1.10":
            case "1.11":
            case "1.12":
                return true;
            default:
                return false;
        }
    }

    private static String majorMinor(String version) {
        int firstDot = version.indexOf('.');
        if (firstDot == -1) return version;
        int secondDot = version.indexOf('.', firstDot + 1);
        return secondDot == -1 ? version : version.substring(0, secondDot);
    }
}
