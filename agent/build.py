import os
import subprocess
import platform

is_windows = platform.system() == "Windows"

if is_windows:
    java_home = r"C:\Program Files\Java\jdk-21.0.11"
    gradlew = "gradlew.bat"
else:
    java_home = "/usr/lib/jvm/java-21-openjdk"
    gradlew = "./gradlew"

env = os.environ.copy()
env["JAVA_HOME"] = java_home

try:
    subprocess.run(
        [gradlew, "build", "jar"],
        env=env,
        check=True
    )
except subprocess.CalledProcessError as e:
    print(f"Build failed with code {e.returncode}")
