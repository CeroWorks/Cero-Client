import os
import sys
import glob
import subprocess
from logger import step, ok, info, warn_, fail_

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
        candidates = glob.glob("/usr/lib/jvm/java-*-openjdk-*") + glob.glob("/usr/lib/jvm/java-*-jdk-*")
        for path in sorted(candidates):
            if os.path.isfile(os.path.join(path, "bin", "java")):
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
        
    jar_mtime = os.path.getmtime(jar_path)
    
    config_files = ["build.gradle", "settings.gradle", "gradle.properties"]
    for f in config_files:
        path = os.path.join(agent_dir, f)
        if os.path.exists(path) and os.path.getmtime(path) > jar_mtime:
            return False
            
    src_dir = os.path.join(agent_dir, "src")
    if not os.path.isdir(src_dir):
        return False
        
    for root, _, files in os.walk(src_dir):
        for f in files:
            if f.endswith((".java", ".kt", ".json", ".toml")):
                file_path = os.path.join(root, f)
                if os.path.getmtime(file_path) > jar_mtime:
                    return False
                    
    return True

def run():
    step("Building Minecraft Agent (Java)...")
    
    agent_dir = os.path.abspath("agent")
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
        
    ok("Agent built successfully")