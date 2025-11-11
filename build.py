import subprocess
import sys
import os

print("/---------- CeroClient  Builder ----------\\")
print("| Choose OS :                             |")
print("| (0) Linux                               |")
print("| (1) Windows                             |")
print("\\-----------------------------------------/")

choice = input("OS : ").strip().lower()

project_dir = os.getcwd()

if choice not in ("linux", "windows"):
    print("[ERROR] Invalid operating system")
    sys.exit(1)

os.chdir(project_dir)

if choice == "windows":
    env = os.environ.copy()
    env["GOOS"] = "windows"
    env["GOARCH"] = "amd64"
    print("[INFO] Building CeroClient for Windows...")
    subprocess.run(["npx", "electron-builder", "--win", "dir"], env=env)

elif choice == "linux":
    env = os.environ.copy()
    env["GOOS"] = "linux"
    env["GOARCH"] = "amd64"
    print("[INFO] Building CeroClient for Linux...")
    subprocess.run(["npx", "electron-builder", "--linux", "dir"], env=env)

print("[INFO] Build finished!")
