package fr.cerostudio;

import fr.cerostudio.service.CeroMixinService;
import org.spongepowered.asm.launch.MixinBootstrap;
import org.spongepowered.asm.mixin.Mixins;
import org.spongepowered.asm.mixin.transformer.IMixinTransformer;
import org.spongepowered.asm.service.MixinService;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLClassLoader;

public class RemappingClassLoader extends URLClassLoader {

    private final String mcVersion;
    private IMixinTransformer mixinTransformer;

    public RemappingClassLoader(URL[] urls, ClassLoader parent, String mcVersion) {
        super(urls, parent);
        this.mcVersion = mcVersion;

        System.out.println("[CeroClassLoader] Démarrage de Mixin en mode ClassLoader pour la " + mcVersion + "...");
        Thread.currentThread().setContextClassLoader(this);

        MixinBootstrap.init();

        String mixinConfig = resolveMixinConfig(mcVersion);
        Mixins.addConfiguration(mixinConfig);

        Object service = MixinService.getService();
        if (service instanceof CeroMixinService) {
            this.mixinTransformer = ((CeroMixinService) service).getTransformer();
        }

        if (this.mixinTransformer != null) {
            System.out.println("[CeroClassLoader] Moteur Mixin hooké avec succès !");
        } else {
            System.err.println("[CeroClassLoader] Erreur: transformer Mixin indisponible.");
        }
    }

    @Override
    protected Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
        synchronized (getClassLoadingLock(name)) {
            Class<?> loadedClass = findLoadedClass(name);
            if (loadedClass != null) return loadedClass;

            if (name.startsWith("net.minecraft.") || 
                name.startsWith("com.mojang.") || 
                name.startsWith("fr.cerostudio.")) {
                
                try {
                    Class<?> c = findClass(name);
                    if (resolve) resolveClass(c);
                    return c;
                } catch (ClassNotFoundException e) {
                    return getParent().loadClass(name);
                }
            }

            try {
                return getParent().loadClass(name);
            } catch (ClassNotFoundException e) {
                Class<?> c = findClass(name);
                if (resolve) resolveClass(c);
                return c;
            }
        }
    }

    @Override
    protected Class<?> findClass(String name) throws ClassNotFoundException {
        String path = name.replace('.', '/') + ".class";
        
        try (InputStream is = this.getResourceAsStream(path)) {
            if (is == null) {
                throw new ClassNotFoundException(name);
            }

            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n;
            while ((n = is.read(buf)) != -1) bos.write(buf, 0, n);
            byte[] rawBytes = bos.toByteArray();

            byte[] transformedBytes = transformWithMixin(name, rawBytes);

            int lastDotIndex = name.lastIndexOf('.');
            if (lastDotIndex != -1) {
                String packageName = name.substring(0, lastDotIndex);
                if (getPackage(packageName) == null) {
                    definePackage(packageName, null, null, null, null, null, null, null);
                }
            }

            Class<?> clazz = defineClass(name, transformedBytes, 0, transformedBytes.length);
            resolveClass(clazz);
            return clazz;

        } catch (IOException e) {
            throw new ClassNotFoundException(name, e);
        }
    }

    public byte[] readResourceBytes(String path) {
        try (InputStream is = this.getResourceAsStream(path)) {
            if (is == null) return null;
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n;
            while ((n = is.read(buf)) != -1) bos.write(buf, 0, n);
            return bos.toByteArray();
        } catch (IOException e) {
            return null;
        }
    }

    private byte[] transformWithMixin(String name, byte[] bytes) {
        if (name.startsWith("fr.cerostudio.")) return bytes;

        if (mixinTransformer != null) {
            try {
                byte[] result = mixinTransformer.transformClassBytes(name, name, bytes);
                return result != null ? result : bytes;
            } catch (Exception e) {
                System.err.println("[CeroClassLoader] Erreur de transformation Mixin pour " + name);
                e.printStackTrace();
            }
        }
        return bytes;
    }

    private String resolveMixinConfig(String version) {
        String majorVersion = version;
        int firstDot = version.indexOf('.');
        if (firstDot != -1) {
            int secondDot = version.indexOf('.', firstDot + 1);
            majorVersion = (secondDot != -1) ? version.substring(0, secondDot) : version;
        }

        if (majorVersion.startsWith("1.7") || majorVersion.startsWith("1.8") || 
            majorVersion.startsWith("1.9") || majorVersion.startsWith("1.10") || 
            majorVersion.startsWith("1.11") || majorVersion.startsWith("1.12")) {
            System.out.println("[CeroClassLoader] Chargement des mixins Legacy (LWJGL 2)");
            return "mixins.cero.v1_legacy.json";
        }

        System.out.println("[CeroClassLoader] Chargement des mixins Modern (LWJGL 3)");
        return "mixins.cero.v1_modern.json";
    }
}