import os
import sys
import shutil
import subprocess
from logger import step, ok, info, warn_, fail_

def run():
    step("Building Bootstrapper (Rust)...")
    
    bs_dir = os.path.abspath("bootstrapper")
    if not os.path.isdir(bs_dir):
        warn_("Bootstrapper directory not found. Skipping.")
        return

    if sys.platform == "win32":
        bin_name = "ceroclient-bootstrapper.exe"
        dest_dir = "bin/windows/bootstrapper"
    elif sys.platform == "darwin":
        bin_name = "ceroclient-bootstrapper"
        dest_dir = "bin/macos/bootstrapper"
    elif sys.platform.startswith("freebsd"):
        bin_name = "ceroclient-bootstrapper"
        dest_dir = "bin/freebsd/bootstrapper"
    else:
        bin_name = "ceroclient-bootstrapper"
        dest_dir = "bin/linux/bootstrapper"

    info("Running cargo build --release...")
    result = subprocess.run(["cargo", "build", "--release"], cwd=bs_dir)
    
    if result.returncode != 0:
        fail_("Bootstrapper build failed")
        
    src_path = os.path.join(bs_dir, "target", "release", bin_name)
    if not os.path.exists(src_path):
        fail_(f"{bin_name} not found after cargo build.")
        
    os.makedirs(dest_dir, exist_ok=True)
    dest_path = os.path.join(dest_dir, bin_name)
    if os.path.exists(dest_path):
        os.remove(dest_path)
    shutil.copy2(src_path, dest_path)
    
    ok(f"Bootstrapper installed in {dest_path}")