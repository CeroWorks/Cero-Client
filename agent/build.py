import os
import subprocess
import platform
import shutil

is_windows = platform.system() == "Windows"

# ── Détection automatique de JAVA_HOME ────────────────────────────────────
# Priorité : variable d'env > java-21 > java-17 > java-8
def find_java_home():
    # 1) Respecter la variable d'environnement si déjà définie
    env_home = os.environ.get("JAVA_HOME", "").strip()
    if env_home and os.path.isdir(env_home):
        return env_home

    # 2) Chercher un JDK 21, 17 ou 8 dans /usr/lib/jvm/
    jvm_dir = "/usr/lib/jvm"
    if os.path.isdir(jvm_dir):
        candidates = []
        for entry in sorted(os.listdir(jvm_dir)):
            full = os.path.join(jvm_dir, entry)
            if not os.path.isdir(full) or entry.startswith("."):
                continue
            # Préférer les JDK (contenant javac)
            javac = os.path.join(full, "bin", "javac" + (".exe" if is_windows else ""))
            has_javac = os.path.isfile(javac)
            candidates.append((full, has_javac))

        # Trier par priorité : JDK 21 > JDK 17 > JDK 8, et préférer ceux avec javac
        def priority(c):
            path, has_javac = c
            p = 0
            if has_javac: p += 100
            low = path.lower()
            if "21" in low: p += 30
            elif "17" in low: p += 20
            elif "8" in low or "1.8" in low: p += 10
            return p

        candidates.sort(key=priority, reverse=True)
        if candidates:
            return candidates[0][0]

    # 3) Fallback : chercher 'javac' dans le PATH
    javac_path = shutil.which("javac")
    if javac_path:
        bin_dir = os.path.dirname(javac_path)
        return os.path.dirname(bin_dir)

    return None

java_home = find_java_home()
if java_home is None:
    print("ERREUR: Impossible de trouver un JDK installé.")
    print("Veuillez installer Java 21+ ou définir JAVA_HOME manuellement.")
    exit(1)

print(f"JAVA_HOME détecté : {java_home}")

gradlew = "gradlew.bat" if is_windows else "./gradlew"

env = os.environ.copy()
env["JAVA_HOME"] = java_home

try:
    subprocess.run(
        [gradlew, "build"],
        env=env,
        check=True
    )
    print("Build succeeded — JAR disponible dans build/libs/")
except subprocess.CalledProcessError as e:
    print(f"Build failed with code {e.returncode}")
    exit(1)
