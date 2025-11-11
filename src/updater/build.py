import subprocess
import shutil
import sys
import os

print("/------- CeroClient Updater Builder -------\\")
print("| Choose OS :                             |")
print("| (0) Linux                               |")
print("| (1) Windows                             |")
print("\\-----------------------------------------/")

choice = input("OS : ").strip().lower()

project_dir = os.path.join(os.getcwd(), "src", "updater")
code_dir = os.getcwd()

if choice not in ("linux", "windows"):
    print("[ERROR] Invalid operating system")
    sys.exit(1)

os.chdir(project_dir)

if choice == "windows":
    env = os.environ.copy()
    env["GOOS"] = "windows"
    env["GOARCH"] = "amd64"
    print("[INFO] Building Wails updater for Windows...")
    subprocess.run(["wails", "build", "-platform", "windows/amd64"], env=env)

elif choice == "linux":
    env = os.environ.copy()
    env["GOOS"] = "linux"
    env["GOARCH"] = "amd64"
    print("[INFO] Building Wails updater for Linux...")
    subprocess.run(["wails", "build", "-platform", "linux/amd64"], env=env)

print("[INFO] Build finished!")