import subprocess
import os
import sys
import platform
import shutil

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SOURCE_FILE = os.path.join(SCRIPT_DIR, "src", "main.nim")
OUTPUT_NAME = os.path.join(SCRIPT_DIR, "cero-installer")

def check_command_exists(cmd):
    path = shutil.which(cmd)
    if path:
        return True

    common_paths = ["/usr/local/bin", "/usr/bin", "/opt/local/bin", "/bin"]
    for p in common_paths:
        full_path = os.path.join(p, cmd)
        if os.path.exists(full_path):
            return True

    return False

def check_dependencies():
    if not check_command_exists("nim"):
        print("Error: Nim compiler not found. Please install Nim and ensure it is in your PATH.")
        sys.exit(1)

    if not check_command_exists("pkg-config"):
        print("Error: pkg-config not found. It is required to link GTK4.")
        sys.exit(1)

    try:
        subprocess.run(["pkg-config", "--exists", "gtk4"], check=True)
    except subprocess.CalledProcessError:
        print("Error: GTK4 development libraries not found. Please install them (e.g., 'sudo apt install libgtk-4-dev').")
        sys.exit(1)

def build(mode="release"):
    output_file = OUTPUT_NAME
    if platform.system() == "Windows":
        output_file += ".exe"

    flags = [
        "c",
        f"-o:{output_file}",
        "--mm:arc",
        "--threads:on",
        "-d:ssl",
    ]

    if mode == "release":
        flags.extend(["-d:release", "--opt:speed", "-d:strip"])
        print("Building in RELEASE mode...")
    elif mode == "debug":
        flags.extend(["-d:debug", "--debuginfo", "--lineTrace"])
        print("Building in DEBUG mode...")
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)

    flags.append(SOURCE_FILE)

    command = ["nim"] + flags
    print(f"Running: {' '.join(command)}")

    try:
        subprocess.run(command, check=True)
        print(f"\nBuild successful! Binary created: {output_file}")
    except subprocess.CalledProcessError:
        print("\nBuild failed. Check the compiler output above.")
        sys.exit(1)

if __name__ == "__main__":
    check_dependencies()

    mode = "release"
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()

    if mode not in ["release", "debug"]:
        print("Usage: python build_installer.py [release|debug]")
        sys.exit(1)

    build(mode)
