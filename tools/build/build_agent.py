import os
import sys
import glob
import subprocess
from logger import step, ok, info, warn_, fail_
import shutil
import hashlib

def compute_source_hash(agent_dir):
    hasher = hashlib.sha256()
    src_dir = os.path.join(agent_dir, "src")
    files = []
    for root, _, filenames in os.walk(src_dir):
        for f in filenames:
            if f.endswith((".java", ".kt", ".json", ".toml")):
                files.append(os.path.join(root, f))
    for f in sorted(files):  # tri pour un hash déterministe
        with open(f, "rb") as fh:
            hasher.update(fh.read())
    for cfg in ("build.gradle", "settings.gradle", "gradle.properties"):
        path = os.path.join(agent_dir, cfg)
        if os.path.exists(path):
            with open(path, "rb") as fh:
                hasher.update(fh.read())
    return hasher.hexdigest()

def copy_jar_to_assets(jar_path, assets_dir):
    os.makedirs(assets_dir, exist_ok=True)
    dest = os.path.join(assets_dir, "CeroClient-MC.jar")
    shutil.copy2(jar_path, dest)
    return dest

def find_java_home():
    java_home = os.environ.get("JAVA_HOME")
    if java_home and os.path.isdir(java_home):
        if sys.platform == "win32":
            if os.path.isfile(os.path.join(java_home, "bin", "java.exe")):
                return java_home
        else:
            if os.path.isfile(os.path.join(java_home, "bin", "java")):
                return java_home

    if sys.platform != "win32":
        candidates = (
            glob.glob("/usr/local/openjdk*")
            + glob.glob("/usr/lib/jvm/java-*-openjdk-*")
            + glob.glob("/usr/lib/jvm/java-*-jdk-*")
            + glob.glob("/Library/Java/JavaVirtualMachines/*/Contents/Home")
        )

        def version_key(path):
            name = os.path.basename(path.rstrip("/"))
            digits = "".join(c for c in name if c.isdigit())
            num = int(digits) if digits else 0
            return ({8: 0, 17: 1, 21: 2}.get(num, 3), num)

        for path in sorted(candidates, key=version_key):
            if os.path.isfile(os.path.join(path, "bin", "javac")):
                return path
    return None

def find_built_jar(agent_dir):
    """Cherche le JAR final dans les dossiers de sortie standards de Gradle."""
    libs_dir = os.path.join(agent_dir, "build", "libs")
    if os.path.isdir(libs_dir):
        jars = glob.glob(os.path.join(libs_dir, "*.jar"))
        if jars:
            return max(jars, key=os.path.getmtime)
            
    root_jars = glob.glob(os.path.join(agent_dir, "*.jar"))
    if root_jars:
        return max(root_jars, key=os.path.getmtime)
        
    return None

def is_agent_up_to_date(agent_dir, jar_path):
    if not jar_path or not os.path.exists(jar_path):
        return False
    hash_file = os.path.join(agent_dir, "build", ".source_hash")
    if not os.path.exists(hash_file):
        return False
    current_hash = compute_source_hash(agent_dir)
    with open(hash_file) as f:
        stored_hash = f.read().strip()
    return current_hash == stored_hash

def save_source_hash(agent_dir):
    hash_file = os.path.join(agent_dir, "build", ".source_hash")
    os.makedirs(os.path.dirname(hash_file), exist_ok=True)
    with open(hash_file, "w") as f:
        f.write(compute_source_hash(agent_dir))

def run():
    step("Building Minecraft Agent (Java)...")
    
    agent_dir = os.path.abspath("agent")
    assets_dir = os.path.abspath("assets")

    if not os.path.isdir(agent_dir):
        fail_("Agent directory not found.")
        
    is_windows = (sys.platform == "win32")
    gradlew_name = "gradlew.bat" if is_windows else "gradlew"
    gradlew_path = os.path.join(agent_dir, gradlew_name)
    
    if not os.path.exists(gradlew_path):
        fail_(f"{gradlew_name} not found in {agent_dir}")
        
    if not is_windows:
        os.chmod(gradlew_path, 0o755)
    
    jar_path = find_built_jar(agent_dir)
    if is_agent_up_to_date(agent_dir, jar_path):
        ok(f"Agent is already up-to-date (JAR: {os.path.basename(jar_path)}). Skipping Gradle build.")
        copy_jar_to_assets(jar_path, assets_dir)
        return
        
    info("Sources have changed, running Gradle build...")
    
    env = os.environ.copy()
    java_home = find_java_home()
    if java_home:
        env["JAVA_HOME"] = java_home
        info(f"Using JAVA_HOME: {java_home}")
    else:
        warn_("JAVA_HOME not found. Gradle might fail if not in system PATH.")

    result = subprocess.run([gradlew_path, "build", "--no-daemon"], cwd=agent_dir, env=env)
    
    if result.returncode != 0:
        fail_("Agent build failed")
        
    jar_path = find_built_jar(agent_dir)
    if not jar_path:
        warn_("Build succeeded but no JAR was found in build/libs/.")
    else:
        copy_jar_to_assets(jar_path, assets_dir)
        ok("Agent built successfully")
        
    ok("Agent built successfully")